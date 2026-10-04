#include "lib/util.h"
#include "lib/json_topology.h"
#include <sys/wait.h>
#include <unistd.h>
#include "lib/netflow.h"
#include "cmockery_horse.h"

static struct netflow *new_nf(void)
{
    struct netflow *nf = malloc(sizeof(struct netflow));
    memset(nf, 0x0, sizeof(struct netflow));
    nf->tags.top = STACK_EMPTY;
    return nf;
}

void push_vlan_empty(void **state)
{
    (void)state;
    struct netflow *nf = new_nf();
    uint16_t tag;
    nf->match.eth_type = 0x800;
    netflow_push_vlan(nf, ETH_TYPE_VLAN);
    assert_int_equal(nf->tags.top, 0);
    tag = nf->tags.level[nf->tags.top].vlan_tag.tag;
    assert_int_equal(tag, 0);
    /* Type should not change */
    assert_int_equal(nf->match.eth_type, 0x800);
    free(nf);
}

void push_mpls_empty(void **state)
{
    (void)state;
    struct netflow *nf = new_nf();
    uint32_t fields;
    nf->match.eth_type = 0x800;
    netflow_push_mpls(nf, ETH_TYPE_MPLS);
    assert_int_equal(nf->tags.top, 0);
    fields = nf->tags.level[nf->tags.top].mpls_tag.fields;
    /* Label should have only the MPLS BOS set */
    assert_int_equal(fields, 0x00000100);
    assert_int_equal(nf->match.mpls_label, 0);
    assert_int_equal(nf->match.mpls_tc, 0);
    assert_int_equal(nf->match.mpls_bos, 1);
    /* Type should change */
    assert_int_equal(nf->match.eth_type, ETH_TYPE_MPLS);
    free(nf);
}

void push_vlan_stack(void **state)
{
    (void)state;
    struct netflow *nf = new_nf();
    uint16_t tag;
    nf->match.eth_type = 0x800;
    netflow_push_vlan(nf, ETH_TYPE_VLAN);
    nf->match.vlan_id = 42;
    /* Set tag value (42 << 2) */
    nf->tags.level[nf->tags.top].vlan_tag.tag = nf->match.vlan_id << 2;
    assert_int_equal(nf->tags.top, 0);

    /* Push a second vlan tag*/
    netflow_push_vlan(nf, ETH_TYPE_VLAN_QinQ);
    assert_int_equal(nf->tags.top, 1);
    tag = nf->tags.level[nf->tags.top].vlan_tag.tag;
    /* Values from first tag must be copied */
    assert_int_equal(tag, 0xA8);
    assert_int_equal(nf->match.vlan_id, 42);
    assert_int_equal(nf->match.eth_type, 0x800);
    free(nf);
}

void push_mpls_stack(void **state)
{
    (void)state;
    struct netflow *nf = new_nf();
    uint32_t fields;
    nf->match.eth_type = 0x800;
    netflow_push_mpls(nf, ETH_TYPE_MPLS);
    /* Set label */
    nf->match.mpls_label = 42;
    fields = nf->match.mpls_label << 12;
    nf->tags.level[nf->tags.top].mpls_tag.fields = fields;
    assert_int_equal(nf->tags.top, 0);
    /* Label should have only the MPLS BOS set */
    assert_int_equal(nf->match.mpls_bos, 1);
    /* Type should change */
    assert_int_equal(nf->match.eth_type, ETH_TYPE_MPLS);

    /* Push second label */
    netflow_push_mpls(nf, ETH_TYPE_MPLS);
    assert_int_equal(nf->tags.top, 1);
    assert_int_equal(nf->match.mpls_label, 42);
    fields = nf->tags.level[nf->tags.top].mpls_tag.fields;
    assert_int_equal(fields, nf->match.mpls_label << 12);
    free(nf);
}

void pop_vlan(void **state)
{
    (void)state;
    struct netflow *nf = new_nf();
    uint16_t tag;
    nf->match.eth_type = 0x800;
    netflow_push_vlan(nf, ETH_TYPE_VLAN);
    nf->match.vlan_id = 42;
    /* Set tag value (42 << 2) */
    nf->tags.level[nf->tags.top].vlan_tag.tag = nf->match.vlan_id << 2;
    assert_int_equal(nf->tags.top, 0);
    /* Pop */
    netflow_pop_vlan(nf);
    assert_int_equal(nf->tags.top, STACK_EMPTY);
    /* Check if the previous value is set to 0 */
    tag = nf->tags.level[nf->tags.top + 1].vlan_tag.tag;
    assert_int_equal(tag, 0);
    assert_int_equal(nf->match.vlan_id, 0);
    free(nf);
}

void pop_mpls(void **state)
{
    (void)state;
    struct netflow *nf = new_nf();
    uint32_t fields;
    nf->match.eth_type = 0x800;
    netflow_push_mpls(nf, ETH_TYPE_MPLS);
    /* Set label */
    nf->match.mpls_label = 42;
    fields = nf->match.mpls_label << 12;
    nf->tags.level[nf->tags.top].mpls_tag.fields = fields;
    assert_int_equal(nf->tags.top, 0);

    /*POP */
    netflow_pop_mpls(nf, 0x800);
    assert_int_equal(nf->tags.top, STACK_EMPTY);
    /* Check if previous field was reset */
    fields = nf->tags.level[nf->tags.top + 1].mpls_tag.fields;
    assert_int_equal(fields, 0);
    /* Other fields */
    assert_int_equal(nf->match.mpls_label, 0);
    assert_int_equal(nf->match.mpls_tc, 0);
    assert_int_equal(nf->match.mpls_bos, 0);
    free(nf);
}

void ipv6_address_compare_uses_full_address(void **state)
{
    (void)state;
    char first[] = "2001:db8::1";
    char second[] = "2001:db8::2";
    assert_int_not_equal(ip_str_addr_compare(first, second, AF_INET6), 0);
    assert_int_equal(ip_str_addr_compare(first, first, AF_INET6), 0);
}

void ipv4_tcp_hash_preserves_wire_bytes(void **state)
{
    (void)state;
    struct netflow *nf = new_nf();
    nf->match.eth_type = ETH_TYPE_IP;
    nf->match.ipv4_src = 0x0a000001;
    nf->match.ipv4_dst = 0x0a000002;
    nf->match.ip_proto = IP_PROTO_TCP;
    nf->match.tcp_src = 1234;
    nf->match.tcp_dst = 80;
#if __BYTE_ORDER == __BIG_ENDIAN
    assert_int_equal(netflow_calculate_hash(nf), 0xa7fb2f21);
#else
    assert_int_equal(netflow_calculate_hash(nf), 0xe92828c1);
#endif
    free(nf);
}

void malformed_json_error_key_is_bounded(void **state)
{
    (void)state;
    pid_t child = fork();
    assert_true(child >= 0);
    if (child == 0) {
        struct parsed_topology topology = {0};
        char *malformed = strdup("{invalid");
        parse_topology(malformed, strlen(malformed), &topology);
        _exit(EXIT_SUCCESS);
    }
    int status = 0;
    assert_int_equal(waitpid(child, &status, 0), child);
    assert_true(WIFEXITED(status));
    assert_int_equal(WEXITSTATUS(status), EXIT_FAILURE);
}

int main(void)
{
    const UnitTest tests[] = {
        unit_test(push_vlan_empty),
        unit_test(push_mpls_empty),
        unit_test(push_vlan_stack),
        unit_test(push_mpls_stack),
        unit_test(pop_vlan),
        unit_test(pop_mpls),
        unit_test(ipv6_address_compare_uses_full_address),
        unit_test(ipv4_tcp_hash_preserves_wire_bytes),
        unit_test(malformed_json_error_key_is_bounded),
    };
    return run_tests(tests);
}