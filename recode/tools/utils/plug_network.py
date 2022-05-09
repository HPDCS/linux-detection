import os
import requests
from .cmd import cmd
from .printer import *
from pathlib import Path
import json



PLUGIN_NAME = "network"
HELP_DESC = "Basic interface to connect Recode to WEB Service"

BASE_URL = 'https://app-review-backend.herokuapp.com/api'
# BASE_URL = 'http://127.0.0.1:8001/api'


def setParserArguments(parser):

    plug_parser = parser.add_parser(PLUGIN_NAME, help=HELP_DESC)

    plug_parser.add_argument(
        "-l",
        "--login",
        nargs=2,
        type=str,
        help="Require <user> and <password>. Retrieve accessToken for Web Service",
    )

    plug_parser.add_argument(
        "-tc",
        "--test-connection",
        action="store_true",
        required=False,
        help="Test connection to Backend by fetching User details (email)",
    )

    plug_parser.add_argument(
        "-s",
        "--send",
        metavar="F",
        nargs='?',
        type=str,
        help="Upload the profile on Review Web",
    )

    plug_parser.add_argument(
        "-t",
        "--type",
        metavar="F",
        nargs='?',
        type=str,
        help="Add type to uploading data",
    )


def action_login(config, args):

    if (len(args) != 2):
        pr_err("Required <username> and <password> to login.")
        return

    response = requests.post(
        BASE_URL + "/auth/login", data={"email": args[0], "password": args[1]})

    # Success
    if (response.status_code == 200):
        responseJson = response.json()
        token = responseJson['token']
        config.update('token', token)
        pr_succ("Login with success")
    else:
        pr_err("Login error..")
        print(response)


def getAuthHeader(config):
    token = config.read('token')
    if (token is None):
        action_login(config)
    return {'Authorization': 'bearer ' + token}


def action_test_connection(config):
    response = requests.get(
        BASE_URL + "/auth/", headers=getAuthHeader(config))

    # Success
    if (response.status_code == 200):
        responseJson = response.json()
        email = responseJson['user']['email']
        print("email: " + str(email))
    else:
        print(response)
        # print(response.json())


def action_send(config, args, type=None):

    pr_info("Reading data from " + str(args))

    file = open(args)
    jsonData = json.load(file)
    rawData = json.dumps(jsonData)
    file.close()
    
    requestData = {}
    requestData['machine'] = config.read('machine')
    requestData['description'] = 'Uploaded from ReCode CLI'
    requestData['type'] = type if type else config.read('type')
    requestData['data'] = rawData

    response = requests.post(
        BASE_URL + "/profile", data=requestData, headers=getAuthHeader(config))

    # Success
    if (response.status_code == 201):
        pr_succ("Upload done ")
    else:
        pr_err("Upload error... (" + str(response) + ")")


def validate_args(args):
    if args.command != PLUGIN_NAME:
        return False

    return True


def compute(args, config):
    if not validate_args(args):
        return False

    # Folder up
    # os.chdir(Path(os.getcwd()).parent)

    # if (args.unload):
    #     action_unload()

    # if (args.clean_compile):
    #     action_clean_compile()

    # if (args.compile):
    #     action_compile()

    if (args.login):
        action_login(config, args.login)

    if (args.test_connection):
        action_test_connection(config)

    if (args.send):
        action_send(config, args.send, args.type)

    # if (args.load_debug):
    #     action_load_debug()

    return True
