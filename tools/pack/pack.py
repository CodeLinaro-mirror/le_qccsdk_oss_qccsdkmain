#===============================================================================
# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#===============================================================================
import os
import shutil
import errno
import stat
import traceback
import glob

verbose_output = True

class PackOpCreateFolder(object):
    def __init__(self, folder_names=[]):
        self._folder_names = folder_names

    def execute(self, build_root_path, sdk_root_path, ignore_errors=False, message_prefix=None):
        for temp_folder_name in self._folder_names:

            # Concatenate with the base relative path
            temp_absolute_dest_path = os.path.join(sdk_root_path, temp_folder_name)

            # The relative path is computed merely for printing out user-friendly messages
            temp_relative_dest_path = os.path.relpath(temp_absolute_dest_path, build_root_path)

            # Create the folder
            create_folder_if_necessary(temp_absolute_dest_path, message_prefix=message_prefix + ' [CreateFolder]', relative_path_message=temp_relative_dest_path, ignore_errors=ignore_errors)

class PackOpCopyFolder(object):
    def __init__(self, source_path, dest_path=None, file_inclusion_list=[], file_exclusion_list=[], include_subfolders=False, folder_exclusion_list=[], extension_inclusion_list=[], flatten_subfolders=False):
        self._source_path = source_path

        if dest_path is not None:
            self._dest_path = dest_path
        else:
            # Split the path into individual directories
            temp_path_split = self._source_path.split(os.path.sep)

            # Use the final directory name as the default destination folder name
            self._dest_path = temp_path_split[-1]

        self._file_exclusion_list = file_exclusion_list
        self._include_subfolders = include_subfolders
        self._file_inclusion_list = file_inclusion_list
        self._folder_exclusion_list = folder_exclusion_list
        self._extension_inclusion_list = extension_inclusion_list
        self._flatten_subfolders = flatten_subfolders

    def execute(self, build_root_path, sdk_root_path, ignore_errors=False, message_prefix=None):

        temp_absolute_source_path = os.path.join(build_root_path, self._source_path)
        temp_absolute_dest_path = os.path.join(sdk_root_path, self._dest_path)

        temp_relative_source_path = os.path.relpath(temp_absolute_source_path, build_root_path)
        temp_relative_dest_path = os.path.relpath(temp_absolute_dest_path, build_root_path)

        temp_folder_list = []
        temp_file_count = 0

        if not self._include_subfolders:
            output_message('[CopyNoSubFolders] Begin copying from "{}" to "{}"...'.format(temp_relative_source_path, temp_relative_dest_path), message_prefix=message_prefix)
            create_folder_if_necessary(temp_absolute_dest_path, message_prefix=message_prefix + ' [CopyNoSubFolders]', relative_path_message=temp_relative_dest_path, ignore_errors=ignore_errors)

            temp_file_list = [f for f in os.listdir(temp_absolute_source_path) if os.path.isfile(os.path.join(temp_absolute_source_path, f))]
            for temp_file in temp_file_list:

                exclude = False

                # Check the file exclusion list to make sure we aren't expected to skip this file
                if any(temp_exclude.lower() in temp_file.lower() for temp_exclude in self._file_exclusion_list):
                    # Exclude this file because it matches the file exclusion list
                    output_message('[CopyNoSubFolders] Excluding file "{}" from copy (reason: matched file exclusion list.)'.format(temp_file.lower()), message_prefix=message_prefix)
                    exclude = True

                # Check the list of included file substrings to make sure we are supposed to include this file
                if not exclude:
                    if len(self._file_inclusion_list) > 0:
                        # If a file inclusion list is present, enforce that the file must match at least one element in the list
                        if not any(temp_file_substring.lower() in temp_file.lower() for temp_file_substring in self._file_inclusion_list):
                            # Exclude this file for failure to match an element in the file  inclusion list
                            output_message('[CopyNoSubFolders] Excluding file "{}" from copy (reason: failed to match file inclusion list.)'.format(temp_file.lower()), message_prefix=message_prefix)
                            exclude = True

                # Check the list of included extensions to make sure we are supposed to include this file
                if not exclude:
                    if len(self._extension_inclusion_list) > 0:
                        # If an extension list is present, enforce that the file must match at least one of them
                        if not any(temp_file.lower().endswith(temp_extension.lower()) for temp_extension in self._extension_inclusion_list):
                            # Exclude this file for failure to match an element in the extension inclusion list
                            output_message('[CopyNoSubFolders] Excluding file "{}" from copy (reason: failed to match extension inclusion list.)'.format(temp_file.lower()), message_prefix=message_prefix)
                            exclude = True

                if not exclude:
                    # A reason to exclude this file was not found; proceed with the copy
                    temp_absolute_source_file = os.path.join(temp_absolute_source_path, temp_file)
                    temp_absolute_dest_file = os.path.join(temp_absolute_dest_path, temp_file)
                    output_message('[CopyNoSubFolders] Copying from "{}" to "{}"...'.format(os.path.join(temp_relative_source_path, temp_file), os.path.join(temp_relative_dest_path, temp_file)), message_prefix=message_prefix)

                    if os.path.isfile(temp_absolute_dest_file):
                        # File already exists; make sure it isn't read-only
                        clear_read_only(temp_absolute_dest_file, message_prefix=message_prefix + ' [CopyNoSubFolders]', ignore_errors=ignore_errors)

                    try:
                        shutil.copy2(temp_absolute_source_file, temp_absolute_dest_file)
                    except Exception as ex:
                        error_message('[CopyNoSubFolders] Exception encountered during attempt to copy from "{}" to "{}"!'.format(os.path.join(temp_relative_source_path, temp_file), os.path.join(temp_relative_dest_path, temp_file)), message_prefix=message_prefix, ignore_errors=ignore_errors, exception=ex)

                    # Clear the read-only flag on the output file
                    clear_read_only(temp_absolute_dest_file, message_prefix=message_prefix + ' [CopyNoSubFolders]', ignore_errors=ignore_errors)

                    # Increment the total file count
                    temp_file_count += 1

            # If we did anything, increment the total folder count
            if temp_file_count > 0:
                if not temp_relative_dest_path in temp_folder_list:
                    temp_folder_list.append(temp_relative_dest_path)

                output_message('[CopyNoSubFolders] Successfully copied {} files to "{}".'.format(temp_file_count, temp_relative_dest_path), message_prefix=message_prefix, is_critical=False)

        else:

            temp_function_prefix = '[CopyWithSubFolders]'
            temp_file_count, temp_folder_list = recursive_copy(build_root_path,
                                                               temp_absolute_source_path,
                                                               temp_absolute_dest_path,
                                                               temp_relative_source_path,
                                                               temp_relative_dest_path,
                                                               self._folder_exclusion_list,
                                                               self._file_inclusion_list,
                                                               self._file_exclusion_list,
                                                               self._extension_inclusion_list,
                                                               self._flatten_subfolders,
                                                               message_prefix,
                                                               temp_function_prefix,
                                                               ignore_errors)

        return (temp_file_count, temp_folder_list)

class PackOpCopySDKFolder(object):
    def __init__(self, source_path, dest_path, file_inclusion_list=[], file_exclusion_list=[], folder_exclusion_list=[], extension_inclusion_list=[], flatten_subfolders=False):
        self._source_path = source_path
        self._dest_path = dest_path
        self._file_inclusion_list = file_inclusion_list
        self._file_exclusion_list = file_exclusion_list
        self._folder_exclusion_list = folder_exclusion_list
        self._extension_inclusion_list = extension_inclusion_list
        self._flatten_subfolders = flatten_subfolders

    def execute(self, build_root_path, sdk_root_path, ignore_errors=False, message_prefix=None):
        temp_absolute_source_path = self._source_path
        temp_absolute_dest_path = self._dest_path
        temp_relative_source_path = os.path.relpath(temp_absolute_source_path, build_root_path)
        temp_relative_dest_path = os.path.relpath(temp_absolute_dest_path, build_root_path)
        temp_function_prefix = '[CopyExistingSDKFolders]'

        temp_file_count, temp_folder_list = recursive_copy(build_root_path,
                                                           temp_absolute_source_path,
                                                           temp_absolute_dest_path,
                                                           temp_relative_source_path,
                                                           temp_relative_dest_path,
                                                           self._folder_exclusion_list,
                                                           self._file_inclusion_list,
                                                           self._file_exclusion_list,
                                                           self._extension_inclusion_list,
                                                           self._flatten_subfolders,
                                                           message_prefix,
                                                           temp_function_prefix,
                                                           ignore_errors)

        return (temp_file_count, temp_folder_list)

class PackOpCopyFile(object):
    def __init__(self, source_path, dest_path=None, dest_file=None):

        temp_split = os.path.split(source_path)

        self._source_path = temp_split[0]
        self._source_file = temp_split[1]

        if dest_path is not None:
            temp_split = os.path.split(dest_path)
            self._dest_path = temp_split[0]
            self._dest_file = temp_split[1]
        else:
            self._dest_path = ''
            self._dest_file = self._source_file

    def execute(self, build_root_path, sdk_root_path, ignore_errors=False, message_prefix=None):

        temp_absolute_source_path = os.path.join(build_root_path, self._source_path)
        temp_absolute_dest_path = os.path.join(sdk_root_path, self._dest_path)

        temp_absolute_source_file = os.path.join(temp_absolute_source_path, self._source_file)
        temp_absolute_dest_file = os.path.join(temp_absolute_dest_path, self._dest_file)

        temp_relative_source_path = os.path.relpath(temp_absolute_source_path, build_root_path)
        temp_relative_dest_path = os.path.relpath(temp_absolute_dest_path, build_root_path)

        temp_relative_source_file = os.path.join(temp_relative_source_path, self._source_file)
        temp_relative_dest_file = os.path.join(temp_relative_dest_path, self._dest_file)

        create_folder_if_necessary(temp_absolute_dest_path, message_prefix=message_prefix + ' [CopyFile]', relative_path_message=temp_relative_dest_path, ignore_errors=ignore_errors)

        if os.path.isfile(temp_absolute_dest_file):
            # File already exists; make sure it isn't read-only
            clear_read_only(temp_absolute_dest_file, message_prefix=message_prefix + ' [CopyFile]', ignore_errors=ignore_errors)
        try:
            # Attempt the file copy
            shutil.copy2(temp_absolute_source_file, temp_absolute_dest_file)
        except Exception as ex:
            # Copy failed
            error_message('Warning: Failed to copy "{}" to "{}"!'.format(temp_relative_source_file, temp_relative_dest_file), message_prefix=message_prefix, ignore_errors=ignore_errors, exception=ex)
            return (0, [])

        output_message('[CopyFile] Successfully copied from "{}" to "{}".'.format(temp_relative_source_file, temp_relative_dest_file), message_prefix=message_prefix, is_critical=False)

        # Clear the read-only flag on the dest file
        clear_read_only(temp_absolute_dest_file, message_prefix=message_prefix + ' [CopyFile]', ignore_errors=ignore_errors)

        return (1, [temp_relative_dest_path])

class Pack(object):
    def __init__(self, build_root_path, sdk_root_path, ignore_errors=False, message_prefix=None):
        '''
        Initializer for a Pack object.

        :param build_root_path:     Root of the build. All source paths are relative to this location.
        :param sdk_root_path:       Root of the SDK ouput folder. All dest paths are relative to this location.
        :param ignore_errors:       Indicates whether any errors encountered should avoid raising exceptions.
        :param message_prefix:      Prefix that is prepended to any output or error messages.
        '''
        self._build_root_path = build_root_path
        self._sdk_root_path = sdk_root_path
        self._sdk_relative_path = os.path.relpath(self._sdk_root_path, self._build_root_path)
        self._ignore_errors = ignore_errors
        self._message_prefix = message_prefix
        self._output_message('SDK pack object initialized.', emphasis=True, is_critical=False)
        self._output_message('   Ignoring errors: .......................... {}'.format(self._ignore_errors), is_critical=False)
        self._output_message('   Source path (existing build root): ........ {}'.format(self._build_root_path), is_critical=False)
        self._output_message('   Dest path (output SDK root): .............. {}'.format(self._sdk_root_path), is_critical=False)
        self._output_message('   Dest path (output SDK root) (relative): ... {}'.format(self._sdk_relative_path), is_critical=False)

    def _error_message(self, message, exception=None):
        error_message(message, message_prefix=self._message_prefix, ignore_errors=self._ignore_errors, exception=exception)

    def _output_message(self, message, emphasis=False, is_critical=False):
        output_message(message, self._message_prefix, emphasis=emphasis, is_critical=is_critical)

    def pack(self, pack_list=[]):

        temp_file_count = 0
        temp_folder_list = []
        temp_final_folder_list = []

        self._output_message('Begin packing {} objects into SDK root path = "{}".'.format(len(pack_list), self._sdk_relative_path), emphasis=True, is_critical=False)

        for temp_pack_object in pack_list:
            temp_file_increment = 0
            temp_folder_increment = 0
            if type(temp_pack_object) is PackOpCreateFolder:
                self._output_message('Processing CreateFolder pack operation...')
                temp_pack_object.execute(self._build_root_path, self._sdk_root_path, self._ignore_errors, self._message_prefix)
            elif type(temp_pack_object) is PackOpCopyFolder:
                self._output_message('Processing CopyFolder pack operation...')
                temp_file_increment, temp_folder_list = temp_pack_object.execute(self._build_root_path, self._sdk_root_path, self._ignore_errors, self._message_prefix)
            elif type(temp_pack_object) is PackOpCopyFile:
                self._output_message('Processing Copyfile pack operation...')
                temp_file_increment, temp_folder_list = temp_pack_object.execute(self._build_root_path, self._sdk_root_path, self._ignore_errors, self._message_prefix)
            elif type(temp_pack_object) is PackOpCopySDKFolder:
                self._output_message('Processing CopySDKFolder pack operation...')
                temp_file_increment, temp_folder_list = temp_pack_object.execute(self._build_root_path, self._sdk_root_path, self._ignore_errors, self._message_prefix)
            else:
                self._error_message('Error processing pack operation: encountered unknown object type (={})!'.format(get_type(temp_pack_object)), exception=None)

            temp_file_count += temp_file_increment
            for temp_folder in temp_folder_list:
                if not temp_folder in temp_final_folder_list:
                    temp_final_folder_list.append(temp_folder)

        self._output_message('SDK pack operation copied a grand total of {} files into {} subfolders of "{}".'.format(temp_file_count, len(temp_final_folder_list), self._sdk_relative_path), emphasis=True, is_critical=False)
        output_blank_line(is_critical=False)

def recursive_copy(build_root_path, absolute_source_path, absolute_dest_path, relative_source_path, relative_dest_path, folder_exclusion_list, file_inclusion_list, file_exclusion_list, extension_inclusion_list, flatten_subfolders, message_prefix, function_prefix, ignore_errors):

    temp_folder_list = []
    temp_file_count = 0
    temp_created_output_folder = False

    output_message('{} Copying with subfolders from "{}" to "{}"...'.format(function_prefix, relative_source_path, relative_dest_path), message_prefix=message_prefix)

    for temp_current_source_path, temp_source_subfolder_list, temp_source_file_list in os.walk(absolute_source_path):

        temp_current_file_count = 0

        # Determine the relative folder name to be used (both source and dest)
        if temp_current_source_path != absolute_source_path:
            temp_relative_current_path = os.path.relpath(temp_current_source_path, absolute_source_path)
        else:
            temp_relative_current_path = ''

        temp_final_absolute_source_path = os.path.join(absolute_source_path, temp_relative_current_path)

        if flatten_subfolders:
            # If flatten is True, copy into the specified destination path regardless of the current source subfolder
            temp_final_absolute_dest_path = absolute_dest_path
            # In this case, the flag which tracks whether the output folder has been created remains valid even as the source subfolder changes.
        else:
            # If flatten is false, we copy into a relative path which mirrors the source path
            temp_final_absolute_dest_path = os.path.join(absolute_dest_path, temp_relative_current_path)
            # Every time we visit a new source folder, we need to reset the flag which tracks whether the output folder has been created yet
            temp_created_output_folder = False

        temp_final_relative_source_path = os.path.relpath(temp_final_absolute_source_path, build_root_path)
        temp_final_relative_dest_path = os.path.relpath(temp_final_absolute_dest_path, build_root_path)

        # Check the folder exclusion list to make sure we aren't expected to skip this folder
        exclude = False
        for temp_exclude in folder_exclusion_list:
            if any(temp_exclude.lower() in temp_folder.lower() for temp_folder in temp_relative_current_path.split(os.path.sep)):
                exclude = True
                break

        if not exclude:
            # This folder should be copied
            for temp_file in temp_source_file_list:

                exclude = False
                # Check the file exclusion list to make sure we aren't expected to skip this file
                #if any(temp_exclude in temp_file for temp_exclude in file_exclusion_list):
                if any(temp_exclude.lower() == temp_file.lower() for temp_exclude in file_exclusion_list):

                    # Exclude this file because it matches the file exclusion list
                    output_message('{} Excluding file "{}" from copy (reason: matched file exclusion list.)'.format(function_prefix, temp_file.lower()), message_prefix=message_prefix)
                    exclude = True

                # Check the list of included file substrings to make sure we are supposed to include this file
                if not exclude:
                    if len(file_inclusion_list) > 0:
                        # If a file inclusion list is present, enforce that the file must match at least one element in the list
                        if not any(temp_file_substring.lower() in temp_file.lower() for temp_file_substring in file_inclusion_list):
                            # Exclude this file for failure to match an element in the file  inclusion list
                            output_message('{} Excluding file "{}" from copy (reason: failed to match file inclusion list.)'.format(function_prefix, temp_file.lower()), message_prefix=message_prefix)
                            exclude = True

                if not exclude:
                    # Check the list of included extensions to make sure we are supposed to include this file
                    if len(extension_inclusion_list) > 0:
                        # If an extension list is present, enforce that the file must match at least one of them
                        if not any(temp_file.lower().endswith(temp_extension.lower()) for temp_extension in extension_inclusion_list):
                            # Exclude this file for failure to match an element in the extension inclusion list
                            output_message('{} Excluding file "{}" from copy (reason: failed to match extension inclusion list.)'.format(function_prefix, temp_file.lower()), message_prefix=message_prefix)
                            exclude = True

                if not exclude:
                    # A reason to exclude this file was not found; proceed to verify that the output folder has been created
                    if not temp_created_output_folder:
                        create_folder_if_necessary(temp_final_absolute_dest_path, message_prefix='{} {}'.format(message_prefix, function_prefix), relative_path_message=temp_final_relative_dest_path, ignore_errors=ignore_errors)
                        # Flag that the folder has been created
                        temp_created_output_folder = True

                    temp_absolute_source_file = os.path.join(temp_final_absolute_source_path, temp_file)
                    temp_absolute_dest_file = os.path.join(temp_final_absolute_dest_path, temp_file)
                    output_message('{} Copying from "{}" to "{}"...'.format(function_prefix, os.path.join(temp_final_relative_source_path, temp_file), os.path.join(temp_final_relative_dest_path, temp_file)), message_prefix=message_prefix)

                    if os.path.isfile(temp_absolute_dest_file):
                        # File already exists; make sure it isn't read-only
                        clear_read_only(temp_absolute_dest_file, message_prefix='{} {}'.format(message_prefix, function_prefix), ignore_errors=ignore_errors)

                    try:
                        # Attempt the file copy
                        shutil.copy2(temp_absolute_source_file, temp_absolute_dest_file)

                        # Succeeded; increment the total and current file counts
                        temp_file_count += 1
                        temp_current_file_count += 1

                    except Exception as ex:
                        # Copy failed
                        error_message('{} Exception encountered during attempt to copy from "{}" to "{}"!'.format(function_prefix, os.path.join(temp_final_relative_source_path, temp_file), os.path.join(temp_final_relative_dest_path, temp_file)), message_prefix=message_prefix, ignore_errors=ignore_errors, exception=ex)

                    try:
                        # Clear the read-only flag on the output file
                        clear_read_only(temp_absolute_dest_file, message_prefix='{} {}'.format(message_prefix, function_prefix), ignore_errors=ignore_errors)
                    except Exception as ex:
                        error_message('{} Error setting writeable attribute on file "{}"!'.format(function_prefix, os.path.join(temp_final_relative_dest_path, temp_file)), message_prefix=message_prefix, ignore_errors=ignore_errors, exception=ex)

            # If we did anything, increment the total folder count
            if temp_current_file_count > 0:
                # Increment the total folder count
                if not temp_final_relative_dest_path in temp_folder_list:
                    temp_folder_list.append(temp_final_relative_dest_path)
                output_message('{} Copied {} files to "{}".'.format(function_prefix, temp_current_file_count, temp_final_relative_dest_path), message_prefix=message_prefix)

        else:
            output_message('{} Excluding source folder "{}" from copy (reason: one or more subfolders matched folder exclusion list.)'.format(function_prefix, temp_final_relative_source_path), message_prefix=message_prefix)

    if temp_file_count > 0:
        if flatten_subfolders:
            output_message('{} Copied a total of {} files into "{}".'.format(function_prefix, temp_file_count, relative_dest_path), message_prefix=message_prefix, is_critical=False)
        else:
            output_message('{} Copied a total of {} files into {} subfolders of "{}".'.format(function_prefix, temp_file_count, len(temp_folder_list), relative_dest_path), message_prefix=message_prefix, is_critical=False)

    return (temp_file_count, temp_folder_list)

def output_message(message, message_prefix=None, emphasis=False, is_critical=False):

    global verbose_output

    if not is_critical and not verbose_output:
        return

    if message_prefix is not None:
        temp_final_message = '{} {}'.format(message_prefix, message)
    else:
        temp_final_message = message

    if emphasis:
        temp_dashed_line = ' ' + ('-' * (len(temp_final_message) + 2)) + ' '
        print(temp_dashed_line)
        print('| ' + temp_final_message + ' |')
        print(temp_dashed_line)
    else:
        print(temp_final_message)

def error_message(message, message_prefix=None, ignore_errors=False, exception=None):
    output_message(message, message_prefix=message_prefix, emphasis=False, is_critical=True)
    if not ignore_errors:
        if exception:
            raise
        else:
            raise Exception()

def output_blank_line(is_critical=False):

    global verbose_output

    if not is_critical and not verbose_output:
        return

    print('')

def create_folder_if_necessary(path_to_folder, message_prefix=None, relative_path_message=None, ignore_errors=False):
    '''
    Attempts to create a folder from a specified path, and returns whether the folder was actually created.

    If the path did not previously exist: it is created (function returns True).
    If the path did previously exist: the raised exception is ignored (function returns False).

    Any other type of exception is raised.
    '''

    if relative_path_message is not None:
        final_path_message = relative_path_message
    else:
        final_path_message = path_to_folder

    try:
        # Attempt to create the folder
        os.makedirs(path_to_folder)
        output_message('Created new folder: "{}".'.format(final_path_message), message_prefix=message_prefix)
        return True
    except OSError as ex:
        # Didn't work; check whether this is because the folder already exists.
        if ex.errno == errno.EEXIST:
            # folder already exists.
            output_message('Skipping creation of folder: "{}". (Reason: already exists.)'.format(final_path_message), message_prefix=message_prefix)
            return False
        else:
            # Some other OS error
            error_message('Error creating new folder: "{}" (error code: {})!'.format(final_path_message, ex.errno), message_prefix=message_prefix, ignore_errors=ignore_errors, exception=ex)
    except Exception as ex:
        error_message('Exception while attempting to create new folder: "{}"!'.format(final_path_message), message_prefix=message_prefix, ignore_errors=ignore_errors, exception=ex)

def get_type(variable):

    '''
    Returns a string representation of any arbitrary variable's *type*.

    Example:      (int) -> (str) 'int'
    Example:         [] -> (str) 'list'
    Example: {'A': 144} -> (str) 'dict'
    '''

    return type(variable).__name__

def clear_read_only(filename, message_prefix=None, ignore_errors=False):
    '''
    Clears the read-only attribute from the specified file, if it exists.
    '''

    try:
        os.chmod(filename, (os.stat(filename).st_mode & 0o777) | stat.S_IWRITE)
        return True
    except Exception as ex:
        error_message('Error setting writeable attribute of file "{}"!'.format(filename), message_prefix=message_prefix, ignore_errors=ignore_errors, exception=ex)
        return False


def get_exception_args_string(exception_with_args):

    if exception_with_args is not None:
        try:
            if len(exception_with_args.args) > 0:
                return exception_with_args.args[0]
            else:
                return '' #(Exception has no description)'
        except AttributeError:
            return ''
    else:
        return '(No exception provided)'

def format_exception_traceback(exception_args=None):
    '''
    Splits the default execution traceback into a list of formatted lines.
    '''

    formatted_lines = traceback.format_exc().splitlines()
    output_lines = []
    if exception_args is not None:
        output_lines.append('### [Exception] {} ###'.format(exception_args))
    else:
        output_lines.append('### [Exception] ###')

    for x in formatted_lines:
        output_lines.append('...{}'.format(x))

    return output_lines

def output_exception_traceback(exception_args=None):

    output_lines = format_exception_traceback(exception_args)
    for temp_line in output_lines:
        output_message(temp_line, '[Exception]', is_critical=False)
