"""Check UTF-8 name boundaries through all public native node bindings."""

import tempfile

from horse import Host, Router, SDNSwitch


def check_names(factory):
    accepted = ("short", "", "a" * 31, "é猫", "é" * 15 + "a")
    rejected = ("a" * 32, "a" * 100, "é" * 16, "a\x00b", "\x00")
    for name in accepted:
        node = factory(name)
        assert node.name == name
        for replacement in accepted:
            node.name = replacement
            assert node.name == replacement
        for invalid in rejected:
            previous = node.name
            try:
                node.name = invalid
            except ValueError:
                pass
            else:
                raise AssertionError(f"Accepted invalid name: {invalid!r}")
            assert node.name == previous
    for name in rejected:
        try:
            factory(name)
        except ValueError:
            pass
        else:
            raise AssertionError(f"Constructed node with invalid name: {name!r}")


def main():
    with tempfile.TemporaryDirectory(prefix="horse-node-names-") as directory:
        for factory in (
            Host,
            lambda name: SDNSwitch(name, 42),
            lambda name: Router(name, runDir=directory),
        ):
            check_names(factory)
    print("PASS node name boundaries, UTF-8 round trips, and rejected assignments")


if __name__ == "__main__":
    main()
