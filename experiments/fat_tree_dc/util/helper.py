"""
Helper module for the plot scripts.
"""

import itertools
import os

import matplotlib as m

if os.uname()[0] == "Darwin":
    m.use("MacOSX")
else:
    m.use("Agg")
import math


def read_list(fname, delim=","):
    lines = open(fname)
    ret = []
    for item in lines:
        ls = item.strip().split(delim)
        ls = [
            "0" if e.strip() == "" or e.strip() == "ms" or e.strip() == "s" else e
            for e in ls
        ]
        ret.append(ls)
    return ret


def ewma(alpha, values):
    if alpha == 0:
        return values
    ret = []
    prev = 0
    for v in values:
        prev = alpha * prev + (1 - alpha) * v
        ret.append(prev)
    return ret


def col(n, obj=None, clean=lambda e: e):
    """A versatile column extractor.

    col(n, [1,2,3]) => returns the nth value in the list
    col(n, [ [...], [...], ... ] => returns the nth column in this matrix
    col('blah', { ... }) => returns the blah-th value in the dict
    col(n) => partial function, useful in maps
    """
    if obj is None:

        def f(item):
            return clean(item[n])

        return f
    if isinstance(obj, list):
        if len(obj) > 0 and (isinstance(obj[0], list) or isinstance(obj[0], dict)):
            return list(map(col(n, clean=clean), obj))
    if isinstance(obj, list) or isinstance(obj, dict):
        try:
            return clean(obj[n])
        except Exception:
            print(f'col(...): column "{n}" not found!')
            return None
    # We wouldn't know what to do here, so just return None
    print(f'col(...): column "{n}" not found!')
    return None


def transpose(item):
    return list(zip(*item, strict=False))


def avg(lst):
    return sum(map(float, lst)) / len(lst)


def stdev(lst):
    mean = avg(lst)
    var = avg([(e - mean) ** 2 for e in lst])
    return math.sqrt(var)


def xaxis(values, limit):
    item = len(values)
    return list(
        zip(
            *[(x_y[0] * 1.0 * limit / item, x_y[1]) for x_y in enumerate(values)],
            strict=False,
        )
    )


def grouper(n, iterable, fillvalue=None):
    "grouper(3, 'ABCDEFG', 'x') --> ABC DEF Gxx"
    args = [iter(iterable)] * n
    return itertools.zip_longest(*args, fillvalue=fillvalue)


def cdf(values):
    values.sort()
    prob = 0
    item = len(values)
    x, y = [], []

    for v in values:
        prob += 1.0 / item
        x.append(v)
        y.append(prob)

    return (x, y)


def parse_cpu_usage(fname, nprocessors=8):
    """Returns (user,system,nice,iowait,hirq,sirq,steal) tuples
    aggregated over all processors.  DOES NOT RETURN IDLE times."""

    data = grouper(nprocessors, open(fname).readlines())

    """Typical line looks like:
    Cpu0  :  0.0%us,  1.0%sy,  0.0%ni, 97.0%id,  0.0%wa,  0.0%hi,  2.0%si,  0.0%st
    """
    ret = []
    for collection in data:
        total = [0] * 8
        for cpu in collection:
            usages = cpu.split(":")[1]
            usages = [e.split("%")[0] for e in usages.split(",")]
            for i in range(len(usages)):
                total[i] += float(usages[i])
        total = [t / nprocessors for t in total]
        # Skip idle time
        ret.append(total[0:3] + total[4:])
    return ret


def pc95(lst):
    item = len(lst)
    return sorted(lst)[int(0.95 * item)]


def pc99(lst):
    item = len(lst)
    return sorted(lst)[int(0.99 * item)]


def coeff_variation(lst):
    return stdev(lst) / avg(lst)
