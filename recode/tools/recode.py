#!/bin/python3
import argparse
import configparser
from os.path import dirname, abspath

import utils.plug_autotest as plug_autotest
import utils.plug_module as plug_module
import utils.plug_config as plug_config
import utils.plug_profiler as plug_profiler
import utils.plug_data as plug_data
import utils.plug_network as plug_network
import utils.plug_security as plug_security


PLUGINS = [plug_module, plug_config, plug_profiler, plug_data, plug_network, plug_security]

CONFIG_FILE = 'recode.ini'


class RecodeConfig:
    def __init__(self, file):
        self.file = file
        self.config = None
        self._initConfig()

    def _initConfig(self):
        self.config = configparser.ConfigParser()
        try:
            with open(self.file) as f:
                self.config.read_file(f)
        except IOError:
            None

    """ If value is None -> REMOVE, if key exists -> UPDATE, else -> ADD"""
    def update(self, key, value):
        if not self.config.has_section('config'):
            self.config.add_section('config')

        self.config['config'][key] = value
        try:
            with open(CONFIG_FILE, 'w') as f:
                self.config.write(f)
        except IOError:
            print("Error while writing " + self.file)

    def read(self, key):
        return self.config['config'][key]


def parser_init():
    parser = argparse.ArgumentParser(
        description="Tool to interact with Recode Module")

    subparser = parser.add_subparsers(help="commands", dest="command")

    for p in PLUGINS:
        p.setParserArguments(subparser)

    plug_autotest.setParserArguments(subparser)

    return parser


def compute_plugins(args, config):
    for p in PLUGINS:
        p.compute(args, config)


if __name__ == "__main__":

    """ TODO Define a class Plugin """
    plug_module.init(dirname(dirname(abspath(__file__))))
    plug_profiler.init(dirname(abspath(__file__)))
    
    parser = parser_init()

    args = parser.parse_args()

    config = RecodeConfig(CONFIG_FILE)

    cmdList = plug_autotest.compute(args, config)

    if len(cmdList):
        for cmd in cmdList:
            compute_plugins(parser.parse_args(cmd), config)
    else:
        compute_plugins(args, config)
