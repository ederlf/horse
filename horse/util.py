from collections import namedtuple


def namedtuple_with_defaults(name, fields, defaults=()):
    """Create a tuple with defaults for the trailing fields."""
    return namedtuple(name, fields, defaults=defaults)
