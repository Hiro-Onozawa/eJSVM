#!/usr/bin/env python

import os
import sys
import subprocess
import argparse
import filecmp
import shutil
import configparser
import glob
import platform
import re

TEST_LIST = 'TEST_LIST'
CONFIG_FILE_PATH = './config'
BC_DIR_NAME = 'bc'
EXE_RESULT_DIR_NAME = 'vm_result'
SCRIPT_PATH = os.path.dirname(os.path.abspath(__file__))
REPORT_FILE = 'REPORT.txt'
SYSTEM = platform.uname().system

def to_abs_path(path):
    _path = os.path.expanduser(path)
    if os.path.isabs(_path):
        return _path
    else:
        return os.path.normpath(os.path.join(SCRIPT_PATH, _path))

def make_parser():
    parser = argparse.ArgumentParser(description='This is a test runner for eJS.')
    parser.add_argument('-c',
            default='default',
            help='...')
    parser.add_argument('--vm-path',
            default=None,
            help='specify eJSVM path')
    parser.add_argument('--compiler-path',
            default=None,
            help='specify eJSC (compiler.jar) path')
    parser.add_argument('--vm-opt',
            default=None,
            help='option for eJSVM execution')
    parser.add_argument('--compile-opt',
            help='option for eJSC execution')
    return parser


def get_test_list(file_path):
    test_list = []
    _file_path = to_abs_path(file_path)
    for line in open(_file_path, 'r'):
        test_list.append(line.replace('\n',''))
    return test_list

def get_file_path_from_test_name(test_name, dirname, ext):
    target_dir = os.path.join(SCRIPT_PATH, dirname)
    bname, _ext = os.path.splitext(test_name)
    path = os.path.normpath(os.path.join(target_dir, bname + ext))
    path_dir = os.path.dirname(path)
    if not os.path.exists(path_dir):
        os.makedirs(path_dir)
    return path

def get_sbc_path(conf, f):
    return get_file_path_from_test_name(f, BC_DIR_NAME, '.'+conf['bytecode'])

def get_result_path(f):
    return get_file_path_from_test_name(f, EXE_RESULT_DIR_NAME, '.txt')

def get_ans_path_list(f):
    js_path = os.path.join(SCRIPT_PATH, f)
    ans_path = get_file_path_from_test_name(js_path, EXE_RESULT_DIR_NAME, '.txt')
    basename = os.path.basename(js_path)
    bname, _ext = os.path.splitext(js_path)
    result_list = glob.glob(bname+'.*') # `ls bname.*`

    ptn_pre_SYSTEM = re.compile('system-.*')
    ans_path_list = []
    for txtpath in result_list:
        _, __ext = os.path.splitext(txtpath)

        # exclude ~.js
        if __ext == '.js':
            continue
        sp = os.path.basename(txtpath).split('.')

        if len(sp) == 2:
            # sp[0] must be `bname` and sp[1] must be 'txt'.
            ans_path_list.append(txtpath)
            continue
        else:
            accepted = True
            for i in range(1, len(sp)):
                if ptn_pre_SYSTEM.match(sp[i]):
                    if sp[i] != ('system-'+SYSTEM):
                        accepted = False
                        break
            if accepted:
                ans_path_list.append(txtpath)
    return ans_path_list

def run_ejsc(compiler_path, opt, js_path, sbc_path):
    opt = ' ' + opt if opt else ''
    cmd = 'java -jar ' + compiler_path + ' ' + js_path + ' -o ' + sbc_path + opt
    return subprocess.call(cmd.split())

def run_ejsvm(ejsvm_path, opt, sbc_path, exec_result_path):
    opt = ' ' + opt + ' ' if opt else ' '
    cmd = ejsvm_path + opt + sbc_path + ' > ' +  exec_result_path + ' 2>&1'
    return subprocess.call(cmd, shell=True)

CSI_ESC = '\033['

def str_color_green(s):
    return CSI_ESC+'32m'+s+CSI_ESC+'0m'
def str_color_red(s):
    return CSI_ESC+'31m'+s+CSI_ESC+'0m'

class Report:
    def __init__(self, n, should_print_stdout):
        self.file_name = to_abs_path(REPORT_FILE)
        if os.path.exists(self.file_name):
            os.remove(self.file_name)
        self.n = n
        self.i = 0
        self.should_print_stdout = should_print_stdout
        self.STR_SEP = ','
        print()

    def next(self, test_name):
        self.i = self.i + 1
        if self.should_print_stdout:
            print(CSI_ESC+'1F', end='') # corsor UP 1
            print(test_name+': ')
            print('['+str(self.i)+'/'+str(self.n)+']')

    def report_pass(self, test_name):
        with open(to_abs_path(REPORT_FILE), 'w') as f:
            f.write(test_name+self.STR_SEP+'PASS'+'\n')
        if self.should_print_stdout:
            print(CSI_ESC+'2F', end='') # corsor UP 2
            print(test_name+': '+str_color_green('PASS'), end='\n\n')

    def report_fail(self, test_name, text):
        with open(to_abs_path(REPORT_FILE), 'w') as f:
            f.write(test_name+self.STR_SEP+text+'\n')
        if self.should_print_stdout:
            print(CSI_ESC+'2F', end='') # corsor UP 2
            print(test_name+': '+str_color_red(text), end='\n\n')
    
    def report_exec_fail(self, test_name):
        self.report_fail(test_name, 'EXEC_FAIL')

    def report_compile_fail(self, test_name):
        self.report_fail(test_name, 'COMPILE_FAIL')

    def report_test_fail(self, test_name):
        self.report_fail(test_name, 'TEST_FAIL')


def run_tests(conf, test_list):
    result = 0
    rep = Report(len(test_list), True)
    for t in test_list:
        rep.next(t)
        js_path = to_abs_path(t)
        sbc_path = get_sbc_path(conf, t)
        exec_result_path = get_result_path(t)
        ans_path_list = get_ans_path_list(t)
        bname, ext = os.path.splitext(js_path)
        ans_path = bname + '.txt'
        r1 = run_ejsc(to_abs_path(conf['compiler']), conf['compile-opt'],  js_path, sbc_path)
        if r1 != 0:
            result = 1
            rep.report_compile_fail(t)
            continue
        r2 = run_ejsvm(to_abs_path(conf['vm']), conf['vm-opt'], sbc_path, exec_result_path)
        if r2 != 0:
            result = 1
            rep.report_exec_fail(t)
            continue
        is_passed = False
        for ans_path in ans_path_list:
            if filecmp.cmp(ans_path, exec_result_path):
                is_passed = True
                break
        if is_passed:
            rep.report_pass(t)
        else:
            result = 1
            rep.report_test_fail(t)
            continue
    return result


def read_config_file(config, conf_name):
    c = configparser.ConfigParser()
    c.read(to_abs_path(CONFIG_FILE_PATH))
    if 'vm' in c[conf_name]:
        config['vm'] = to_abs_path(c[conf_name]['vm'])
    if 'compiler' in c[conf_name]:
        config['compiler'] = to_abs_path(c[conf_name]['compiler'])
    if 'vm-opt' in c[conf_name]:
        config['vm-opt'] = c[conf_name]['vm-opt']
    if 'compile-opt' in c[conf_name]:
        config['compile-opt'] = c[conf_name]['compile-opt']

if __name__ == '__main__':
    parser = make_parser()

    config = {
            'vm' : None,
            'vm-opt' : '-l',
            'compiler' : None,
            'compile-opt' : '',
            'bytecode' : 'sbc', # 'sbc'|'obc'
            }
    args = parser.parse_args()
    read_config_file(config, args.c)

    # get arguments
    if args.vm_path is not None:
        config['vm'] = to_abs_path(args.vm_path)
    if args.compiler_path is not None:
        config['compiler'] = to_abs_path(args.compiler_path)
    if args.vm_opt is not None:
        config['vm-opt'] = args.vm_opt
    if args.compile_opt is not None:
        config['compile-opt'] = args.compile_opt
        if re.match(r'.*--out-obc.*', args.compile_opt):
            config['bytecode'] = 'obc'

    # checking arguments
    if config['vm'] is None or config['compiler'] is None:
        parser.print_help()
        sys.exit(1)
    if not os.path.isfile(config['vm']):
        print('vm: ' + config['vm'] + ' is not found.')
        sys.exit(1)
    if not os.path.isfile(config['compiler']):
        print('compiler: ' + config['compiler'] + ' is not found.')
        sys.exit(1)

    print('vm:          ' + config['vm'])
    print('vm-opt:      ' + config['vm-opt'])
    print('compiler:    ' + config['compiler'])
    print('compile-opt: ' + config['compile-opt'])
    print('--------------')

    test_list = get_test_list(TEST_LIST)
    sys.exit(run_tests(config, test_list))
