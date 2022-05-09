import os
import requests
from .cmd import cmd
from .printer import *
from pathlib import Path


PLUGIN_NAME = "security"
HELP_DESC = "Manage the ReCode SECURITY plugin. Can be used only on special Linux Kernels"

RECODE_PROC_PATH = "/proc/recode"


def setParserArguments(parser):

    plug_parser = parser.add_parser(PLUGIN_NAME, help=HELP_DESC)

    plug_parser.add_argument(
        "-m",
        "--mitigations",
        type=str,
        nargs='+',
        metavar="M",
        choices=["te", "exile", "llc", "verbose", "none", "no_nx"],
        required=False,
        help="Enable the Dynamic Mitigations",
    )


def action_mitigations(config, args):
    path = RECODE_PROC_PATH + "/mitigations"

    _file = open(path, "w")

    mask = 0

    #define DM_G_LLC_FLUSH			0	
    #define DM_G_CPU_EXILE			1	
    #define DM_G_TE_MITIGATE		2
    #define DM_G_SKIP_NX_PTI        17
    #define DM_G_VERBOSE			18	

    if "llc" in args:
        mask |= (1 << 0)
    if "exile" in args:
        mask |= (1 << 1)
    if "te" in args:
        mask |= (1 << 2)
    if "no_nx" in args:
        mask |= (1 << 17)
    if "verbose" in args:
        mask |= (1 << 18)
    if "none" in args:
        mask = 0

    _file.write(str(mask))
    _file.flush()
    _file.close()


def validate_args(args):
    if args.command != PLUGIN_NAME:
        return False

    return True


def compute(args, config):
    if not validate_args(args):
        return False

    if (args.mitigations):
        action_mitigations(config, args.mitigations)

    return True
