# ===============================================================================
#
#
# ===============================================================================

def range_check(name, value, min, max):
    if value > max:
        raise RuntimeError("%r cannot be larger than %d" % (name, max))
    elif value < min:
        raise RuntimeError("%r cannot be less than %d" % (name, min))
