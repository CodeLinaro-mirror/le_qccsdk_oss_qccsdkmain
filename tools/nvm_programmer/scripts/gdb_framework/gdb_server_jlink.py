#===============================================================================
# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#===============================================================================
import subprocess
import os
import time

def add_arguments(parser):
    '''
    Adds the arguments needed for the GDB_Server to the command line
    parser.

    Parameters:
       parser: argparse object used for parsing the command line arguments.
    '''

class GDB_Server(object):
    '''
       Class representing the GDB server.
    '''
    def __init__(self, **kwargs):
        '''
        Initializer for a GDB Server object.

        Parameters:
            server_path:     Path for JLinkGDBServerCL.exe.
            server_port:     GDB server port number.
            no_start_server: Flag indicating the server does not need to be started
        '''
        # Get the server path
        if ('server_path' in kwargs) and (kwargs['server_path']):
            # Path is provided as an argument so just use it.
            self.server_path = kwargs['server_path']
        elif 'JLINK_PATH' in os.environ:
            # Get it from an environemnt variable
            self.server_path = os.environ['JLINK_PATH']
        else:
            try:
                # Try to read the path from the registry
                try:
                    import winreg
                except:
                    import _winreg as winreg

                with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, 'Software\SEGGER\J-Link') as regkey:
                    self.server_path, regtype = winreg.QueryValueEx(regkey, 'InstallPath')
            except:
                # Nothing else available so just hard-code it.
                if (os.name == 'posix'):
                    self.server_path = ''
                else:
                    self.server_path = 'C:\Program Files (x86)\SEGGER\JLink'

        self.start_server = kwargs['start_server']

        if ('server_script' in kwargs) and (kwargs['server_script']):
            self.server_script = kwargs['server_script']
        else:
            self.server_script = 'qcc730.JLinkScript'

        self.server_script = self.server_script.replace(os.sep, '/')

        self.server_proc = None

        if (os.name == 'posix'):
            self.executable = 'JLinkGDBServerCLExe'
        else:
            self.executable = 'JLinkGDBServerCL.exe'

        if (os.path.exists(os.path.join(self.server_path, self.executable)) == False):
            raise Exception("The gdb server executable file {} is not exist.".format(os.path.join(self.server_path, self.executable)))

        if ('serial' in kwargs) and (kwargs['serial']):
            self.options = ['-select', 'USB={}'.format(kwargs['serial'])]
        else:
            self.options = ['-select', 'USB']

        self.options += [
            '-device', 'Cortex-M4',
            '-endian', 'little',
            '-if', 'JTAG',
            '-speed', '1000',
            '-JTAGconf', '0,0',
            '-noir',
            '-port', str(kwargs['server_port']),
            '-singlerun',
            '-silent',
            '-jlinkscriptfile'
        ]

    def start(self):
        '''
        Starts the GDB Server.
        '''
        if self.server_proc:
            raise Exception('Server already started.')

        self.options += [
            self.server_script
        ]

        if self.start_server:
            command = [os.path.join(self.server_path, self.executable)] + self.options

            print('Using SEGGER J-Link GDB server')
            print(' '.join(command))
            self.server_proc = subprocess.Popen(command)

            # Wait for the server to be ready
            time.sleep(1)

    def stop(self):
        '''
        Kill the GDB Server.
        '''
        # only kill the server if the script started it.
        if self.server_proc:
            try:
                # Wait for the server to close itself (should happen fairly quickly if
                # the GDB client disconnected)
                self.server_proc.communicate()
            except:
                # Server didn't close so force it.
                try:
                    self.server_proc.terminate()
                except:
                    pass

            self.server_proc = None

    def client_setup(self, client):
        '''
        Performs any required initialization commands that the GDB Server needs the
        GDB client to execute.

        Parameters:
            client: object used to send commands to the gdb_client.
        '''
        #client.execute('gdb.execute("monitor reset")')

