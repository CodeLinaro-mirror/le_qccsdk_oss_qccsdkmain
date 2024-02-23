# ===============================================================================
#
#
# ===============================================================================


def get_signing_config_overrides():
    retval = None
    try:
        from sectools.common.engg.config_overrides import get_config_overrides
        retval = get_config_overrides()
    except ImportError:
        pass
    return retval

