from termcolor import colored


def pr_text(msg):
    print(msg)


def pr_info(msg):
    print(colored(msg, 'blue'))


def pr_succ(msg):
    print(colored(msg, 'green'))


def pr_warn(msg):
    print(colored(msg, 'yellow'))


def pr_err(msg):
    print(colored(msg, 'red'))
