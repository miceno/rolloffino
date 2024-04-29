#!/usr/bin/env python3
#
# Python script to send text messages over a TCP socket.
# Copyright 2023
# by Orestes Sanchez Benavente is licensed under CC BY-NC-SA 4.0.
#
#
# Script to send text messages over a TCP socket.
# It creates a connection to the server and it keeps it
# for the whole session.
#
# The code is a bit around the SnapCap and Rolloffino protocol.
#
# Author: Orestes Sanchez-Benavente <miceno.atreides@gmail.com>
#

import socket
import errno
import argparse


class TcpClient:
    def __init__(self, server, port, *args, **kwargs):
        self._socket = socket.create_connection((server, port), )
        self._encoding = kwargs.get('encoding', 'ascii')

    def _is_last(self, data):
        return "Exit" == data

    def _hook_after_read(self, data):
        return data

    def _hook_before_send(self, data):
        return data

    def get_text(self):
        data = input("Please enter the message: ")
        return data

    def loop(self):
        while True:
            data = self.get_text()
            if (self._is_last(data)):
                return
            data = self._hook_after_read(data)
            data = self._hook_before_send(data)
            self.send_data(data)
            response = self.receive_response()
            print('Received', repr(response))

    def send_data(self, data):
        print(f'Processing Message *****{data!r}*****')
        self._socket.send(data.encode(self._encoding, 'ignore'))

    def is_eof(self, data):
        return ('\n' in data or ')' in data)

    def receive_response(self):
        run_main_loop = True
        while run_main_loop:
            # The socket have data ready to be received
            data = ''
            continue_recv = True

            while continue_recv:
                # Try to receive som data
                data += self._socket.recv(1024).decode()
                continue_recv = not self.is_eof(data)
            run_main_loop = not self.is_eof(data)

        return data

class SnapcapTCP(TcpClient):
    def is_eof(self, data):
        return ('\n' in data)


def plain_loop(s):
    while True:
        data = input("Please enter the message: ")
        if 'Exit' == data:
            break
        data = data.strip('\n')
        print(f'Processing Message from input() *****{data}*****')
        s.send(data.encode('ascii', 'ignore'))
        data = s.recv(1024).decode()
        print('Received', repr(data))


def wait_for_all_loop(s, opts):
    s.settimeout(0.5)
    s.setblocking(True)

    while True:
        data = input("Please enter the message: ")
        if 'Exit' == data:
            break
        if opts.newline:
            print("add new line")
            data += "\n"
        print(f'Processing Message *****{data!r}*****')
        s.send(data.encode('ascii', 'ignore'))

        # time.sleep(1)
        run_main_loop = True
        while run_main_loop:
            # The socket have data ready to be received
            data = ''
            continue_recv = True

            while continue_recv:
                # Try to receive som data
                data += s.recv(1024).decode()
                continue_recv = not ('\n' in data and ')' in data)
            # print(f"data={data}\n")
            run_main_loop = not ('\n' in data and ')' in data)
            # print(f"run_main_loop={run_main_loop}\n")

        print(F'Received           *****{data!r}*****')


def init_argparse() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Send and receive TCP messages over a persistent server connection."
    )
    parser.add_argument(
        "-s", "--server", default="localhost",
        help="Server host"
    )
    parser.add_argument(
        "-n", "--newline", action="store_true", default=False,
        help="Add a new line to every request"
    )
    parser.add_argument(
        "-p", "--port", type=int, default=8888,
        help="Server port"
    )
    return parser


if __name__ == '__main__':
    parser = init_argparse()
    args = parser.parse_args()

    s = socket.create_connection((args.server, args.port), )
    wait_for_all_loop(s, args)

    #client = TcpClient(args.server, args.port)
    #client.loop()

    pass
