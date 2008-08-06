"""Run all particle unit tests"""

__version__ = '$Id$'

import unittest

from controller_test import *
from emitter_test import *
from group_test import *
from system_test import *
from domain_test import *

if __name__ == '__main__':
	unittest.main()
