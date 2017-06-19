#include "lib/netflow.h"
#include "cmockery_horse.h"

static struct netflow *new_nf(){
    struct netflow *nf = malloc(sizeof(struct netflow));
    memset(nf, 0x0, sizeof(struct netflow));
    nf->tags.top = STACK_EMPTY;
    return nf;
}

void push_vlan_empty(void **state) {
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

void push_mpls_empty(void **state) {
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

void push_vlan_stack(void **state) {
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

void push_mpls_stack(void **state) {
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


void pop_vlan(void **state) {
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

void pop_mpls(void **state) {
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

int main(int argc, char* argv[]) {
    const UnitTest tests[] = {
        unit_test(push_vlan_empty),
        unit_test(push_mpls_empty),
        unit_test(push_vlan_stack),
        unit_test(push_mpls_stack),
        unit_test(pop_vlan),
        unit_test(pop_mpls),
    };
    return run_tests(tests);
}