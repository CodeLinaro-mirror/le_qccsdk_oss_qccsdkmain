# ===============================================================================
#
#
# ===============================================================================

from abc import ABCMeta, abstractmethod

from six import add_metaclass


@add_metaclass(ABCMeta)
class BaseEncryptor(object):

    def __init__(self, encryption_parameters, debug_dir=None):
        self.encryption_parameters = encryption_parameters
        self.debug_dir = debug_dir

    @abstractmethod
    def encrypt_segment(self, binary_segment, segment_num):
        raise NotImplementedError()
