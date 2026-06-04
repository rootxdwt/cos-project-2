import argparse
import json
import logging
import socket
import struct
import sys
import threading

import requests

OPCODE_DATA = 1
OPCODE_WAIT = 2
OPCODE_DONE = 3
OPCODE_QUIT = 4


class Server:
    def __init__(self, name, algorithm, port, caddr, cport, ntrain, ntest):
        logging.info(
            "[*] Initializing the server module to receive data from the edge device"
        )
        self.name = name
        self.algorithm = algorithm
        self.dimension = None
        self.index = 0
        self.caddr = caddr
        self.cport = cport
        self.ntrain = ntrain
        self.ntest = ntest
        self.connected_to_ai = False
        self.ai_lock = threading.Lock()

        self.port = port
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.bind(("0.0.0.0", port))
        self.socket.listen(10)
        self.listener()

    def connecter(self):
        success = True
        self.ai = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.ai.connect((self.caddr, self.cport))
        url = "http://{}:{}/{}".format(self.caddr, self.cport, self.name)
        request = {}
        request["algorithm"] = self.algorithm
        request["dimension"] = self.dimension
        request["index"] = self.index
        js = json.dumps(request)
        logging.debug("[*] To be sent to the AI module: {}".format(js))
        result = requests.post(url, json=js)
        response = json.loads(result.content)
        logging.debug("[*] Received: {}".format(response))

        if "opcode" not in response:
            logging.debug("[*] Invalid response")
            success = False
        else:
            if response["opcode"] == "failure":
                logging.error("Error happened")
                if "reason" in response:
                    logging.error("Reason: {}".format(response["reason"]))
                    logging.error("Please try again.")
                else:
                    logging.error("Reason: unknown. not specified")
                success = False
            else:
                assert response["opcode"] == "success"
                logging.info("[*] Successfully connected to the AI module")
        return success

    def listener(self):
        logging.info("[*] Server is listening on 0.0.0.0:{}".format(self.port))

        while True:
            client, info = self.socket.accept()
            logging.info(
                "[*] Server accept the connection from {}:{}".format(info[0], info[1])
            )

            client_handle = threading.Thread(target=self.handler, args=(client,))
            client_handle.start()

    def send_instance(self, vlst, is_training):
        if is_training:
            url = "http://{}:{}/{}/training".format(self.caddr, self.cport, self.name)
        else:
            url = "http://{}:{}/{}/testing".format(self.caddr, self.cport, self.name)
        data = {}
        data["value"] = vlst
        req = json.dumps(data)
        response = requests.put(url, json=req)
        resp = response.json()

        if "opcode" in resp:
            if resp["opcode"] == "failure":
                logging.error("fail to send the instance to the ai module")

                if "reason" in resp:
                    logging.error(resp["reason"])
                else:
                    logging.error("unknown error")
                sys.exit(1)
        else:
            logging.error("unknown response")
            sys.exit(1)

    def recv_exact(self, client, n):
        data = bytearray()
        while len(data) < n:
            packet = client.recv(n - len(data))
            if not packet:
                return None
            data.extend(packet)
        return bytes(data)

    def parse_data(self, format_id, buf, is_training):
        formats = {
            1: (
                ">hhhBhh",
                "c1 [avg_power, temp_avg, humid_avg, month, thi, temp_range]",
            ),
            2: (
                ">hhhhh",
                "c2 [avg_power, min_power, max_power, std_dev_power, papr]",
            ),
            3: (
                ">hhhBhhh",
                "c3 [avg_power, thi, temp_range, month, min_power, std_dev_power, papr]",
            ),
        }

        fmt, desc = formats[format_id]
        lst = list(struct.unpack(fmt, buf))
        logging.info("{} = {}".format(desc, lst))

        self.send_instance(lst, is_training)

    # TODO: You should implement your own protocol in this function
    # The following implementation is just a simple example
    def handler(self, client):
        logging.info("[*] Server starts to process the client's request")

        ntrain = self.ntrain
        url = "http://{}:{}/{}/training".format(self.caddr, self.cport, self.name)

        while True:
            # opcode (1 byte):
            rbuf = self.recv_exact(client, 1)
            if not rbuf:
                logging.error("[*] client disconnected prematurely")
                sys.exit(1)
            opcode = int.from_bytes(rbuf, "big")
            logging.debug("[*] opcode: {}".format(opcode))

            if opcode == OPCODE_DATA:
                logging.info("[*] data report from the edge")

                # Receive Format ID (1 byte)
                fmt_buf = self.recv_exact(client, 1)
                if not fmt_buf:
                    logging.error("[*] failed to receive format ID")
                    sys.exit(1)
                format_id = int.from_bytes(fmt_buf, "big")
                logging.debug("[*] received format ID: {}".format(format_id))

                len_buf = self.recv_exact(client, 2)
                if not len_buf:
                    logging.error("[*] failed to receive data length")
                    sys.exit(1)
                dlen = int.from_bytes(len_buf, "big")
                logging.debug("[*] received data length: {}".format(dlen))

                rbuf = self.recv_exact(client, dlen)
                if not rbuf:
                    logging.error("[*] failed to receive data payload")
                    sys.exit(1)
                logging.debug("[*] received buf: {}".format(rbuf))

                with self.ai_lock:
                    if not self.connected_to_ai:
                        if format_id == 1:
                            self.dimension = 6
                        elif format_id == 2:
                            self.dimension = 5
                        elif format_id == 3:
                            self.dimension = 7

                        success = self.connecter()
                        if not success:
                            logging.error(
                                "[*] Failed to connect and register model in AI module"
                            )
                            sys.exit(1)
                        self.connected_to_ai = True

                self.parse_data(format_id, rbuf, True)
            else:
                logging.error("[*] invalid opcode")
                logging.error("[*] please try again")
                sys.exit(1)

            ntrain -= 1

            if ntrain > 0:
                opcode = OPCODE_DONE
                logging.debug("[*] send the opcode OPCODE_DONE")
                client.send(int.to_bytes(opcode, 1, "big"))
            else:
                opcode = OPCODE_WAIT
                logging.debug("[*] send the opcode OPCODE_WAIT")
                client.send(int.to_bytes(opcode, 1, "big"))
                break

        result = requests.post(url)
        response = json.loads(result.content)
        logging.debug("[*] return: {}".format(response["opcode"]))

        ntest = self.ntest
        url = "http://{}:{}/{}/testing".format(self.caddr, self.cport, self.name)
        opcode = OPCODE_DONE
        logging.debug("[*] send the opcode OPCODE_DONE")
        client.send(int.to_bytes(opcode, 1, "big"))

        while ntest > 0:
            # opcode (1 byte):
            rbuf = self.recv_exact(client, 1)
            if not rbuf:
                logging.error("[*] client disconnected prematurely")
                sys.exit(1)
            opcode = int.from_bytes(rbuf, "big")
            logging.debug("[*] opcode: {}".format(opcode))

            if opcode == OPCODE_DATA:
                logging.info("[*] data report from the edge")

                # Receive Format ID (1 byte)
                fmt_buf = self.recv_exact(client, 1)
                if not fmt_buf:
                    logging.error("[*] failed to receive format ID")
                    sys.exit(1)
                format_id = int.from_bytes(fmt_buf, "big")
                logging.debug("[*] received format ID: {}".format(format_id))

                len_buf = self.recv_exact(client, 2)
                if not len_buf:
                    logging.error("[*] failed to receive data length")
                    sys.exit(1)
                dlen = int.from_bytes(len_buf, "big")
                logging.debug("[*] received data length: {}".format(dlen))

                rbuf = self.recv_exact(client, dlen)
                if not rbuf:
                    logging.error("[*] failed to receive data payload")
                    sys.exit(1)
                logging.debug("[*] received buf: {}".format(rbuf))
                self.parse_data(format_id, rbuf, False)
            else:
                logging.error("[*] invalid opcode")
                logging.error("[*] please try again")
                sys.exit(1)

            ntest -= 1

            if ntest > 0:
                opcode = OPCODE_DONE
                client.send(int.to_bytes(opcode, 1, "big"))
            else:
                opcode = OPCODE_QUIT
                client.send(int.to_bytes(opcode, 1, "big"))
                break

        url = "http://{}:{}/{}/result".format(self.caddr, self.cport, self.name)
        result = requests.get(url)
        response = json.loads(result.content)
        logging.debug("response: {}".format(response))
        if "opcode" not in response:
            logging.error("invalid response from the AI module: no opcode is specified")
            logging.error("please try again")
            sys.exit(1)
        else:
            if response["opcode"] == "failure":
                logging.error("getting the result from the AI module failed")
                if "reason" in response:
                    logging.error(response["reason"])
                logging.error("please try again")
                sys.exit(1)
            elif response["opcode"] == "success":
                self.print_result(response)
            else:
                logging.error("unknown error")
                logging.error("please try again")
                sys.exit(1)

    def print_result(self, result):
        logging.info("=== Result of Prediction ({}) ===".format(self.name))
        logging.info("   # of instances: {}".format(result["num"]))
        logging.debug("   sequence: {}".format(result["sequence"]))
        logging.debug("   prediction: {}".format(result["prediction"]))
        logging.info("   correct predictions: {}".format(result["correct"]))
        logging.info("   incorrect predictions: {}".format(result["incorrect"]))
        logging.info("   accuracy: {}\%".format(result["accuracy"]))


def command_line_args():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-a",
        "--algorithm",
        metavar="<AI algorithm to be used>",
        help="AI algorithm to be used",
        type=str,
        required=True,
    )
    parser.add_argument(
        "-b",
        "--caddr",
        metavar="<AI module's IP address>",
        help="AI module's IP address",
        type=str,
        required=True,
    )
    parser.add_argument(
        "-c",
        "--cport",
        metavar="<AI module's listening port>",
        help="AI module's listening port",
        type=int,
        required=True,
    )
    parser.add_argument(
        "-p",
        "--lport",
        metavar="<server's listening port>",
        help="Server's listening port",
        type=int,
        required=True,
    )
    parser.add_argument(
        "-n",
        "--name",
        metavar="<model name>",
        help="Name of the model",
        type=str,
        default="modelmodel01372835612456",
    )
    parser.add_argument(
        "-x",
        "--ntrain",
        metavar="<number of instances for training>",
        help="Number of instances for training",
        type=int,
        default=10,
    )
    parser.add_argument(
        "-y",
        "--ntest",
        metavar="<number of instances for testing>",
        help="Number of instances for testing",
        type=int,
        default=10,
    )
    parser.add_argument(
        "-l",
        "--log",
        metavar="<log level (DEBUG/INFO/WARNING/ERROR/CRITICAL)>",
        help="Log level (DEBUG/INFO/WARNING/ERROR/CRITICAL)",
        type=str,
        default="INFO",
    )
    args = parser.parse_args()
    return args


def main():
    args = command_line_args()
    logging.basicConfig(level=args.log)

    if args.ntrain <= 0 or args.ntest <= 0:
        logging.error(
            "Number of instances for training or testing should be larger than 0"
        )
        sys.exit(1)

    Server(
        args.name,
        args.algorithm,
        args.lport,
        args.caddr,
        args.cport,
        args.ntrain,
        args.ntest,
    )


if __name__ == "__main__":
    main()
