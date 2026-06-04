import socket
import requests
import threading
import argparse
import logging
import json
import sys

OPCODE_DATA = 1
OPCODE_WAIT = 2
OPCODE_DONE = 3
OPCODE_QUIT = 4

class Server:
    def __init__(self, name, algorithm, dimension, index, port, caddr, cport, ntrain, ntest):
        logging.info("[*] Initializing the server module to receive data from the edge device")
        self.name = name
        self.algorithm = algorithm
        self.dimension = dimension
        self.index = index
        self.caddr = caddr
        self.cport = cport
        self.ntrain = ntrain
        self.ntest = ntest
        success = self.connecter()  # AI 연동 모듈 초기 설정을 담당하는 함수로 TCP 소켓을 생성하여 AI 모듈과 연결하고 사용자가 지정한 알고리즘, 인덱스 정보 등을 JSON 형태로 구성하여 HTTP POST 요청을 통해 AI 모듈에 전송한다.
                                    # AI 모듈로부터 성공 응답을 받을 시 True를 반환한다.

        if success:
            self.port = port
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.bind(("0.0.0.0", port))
            self.socket.listen(10)
            self.listener()  # 외부 디바이스에서의 접속을 수락 및 대기시킨다. 소켓을 열어 클라이언트 연결 요청을 대기하며 새로운 에지 디바이스가 접속하면 멀티스레드를 생성해 해당 클라이언트의 요청 처리를 독립된 handler 함수로 넘긴다.

    def connecter(self):
        success = True 
        # AI 모듈과 통신할 TCP 소켓 생성 및 연결
        self.ai = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.ai.connect((self.caddr, self.cport)) 
        
        # AI 모듈의 REST API 엔드포인트 주소 설정
        url = "http://{}:{}/{}".format(self.caddr, self.cport, self.name)

        # AI 모듈에 등록할 파라미터(알고리즘, 차원, 인덱스)를 딕셔너리로 구성
        request = {}
        request['algorithm'] = self.algorithm
        request['dimension'] = self.dimension
        request['index'] = self.index

        # 딕셔너리를 JSON 문자열로 직렬화(Serialization)
        js = json.dumps(request)
        logging.debug("[*] To be sent to the AI module: {}".format(js))

        # AI 모듈에 HTTP POST 요청으로 설정값 전송 및 응답 수신
        result = requests.post(url, json=js)
        response = json.loads(result.content)
        logging.debug("[*] Received: {}".format(response))

        # 응답 프로토콜 검증 (opcode 필드 존재 여부 확인)
        if "opcode" not in response:
            logging.debug("[*] Invalid response")
            success = False
        else:
            # AI 모듈 설정 실패 시 예외 처리 및 로그 출력
            if response["opcode"] == "failure":
                logging.error("Error happened")
                if "reason" in response:
                    logging.error("Reason: {}".format(response["reason"]))
                    logging.error("Please try again.")
                else:
                    logging.error("Reason: unknown. not specified")
                success = False
            else:
                # 성공 응답인 경우 진행
                assert response["opcode"] == "success"
                logging.info("[*] Successfully connected to the AI module")
        return success

    # 외부 에지(Edge) 디바이스로부터의 접속을 대기하고 수락하는 함수
    def listener(self):
        logging.info("[*] Server is listening on 0.0.0.0:{}".format(self.port))

        # 무한 루프를 돌며 다중 클라이언트 접속을 지속적으로 대기
        while True:
            # 클라이언트의 연결 요청을 수락 (소켓 객체와 주소 정보 반환)
            client, info = self.socket.accept()
            logging.info("[*] Server accept the connection from {}:{}".format(info[0], info[1]))

            # 서버의 블로킹(대기)을 방지하기 위해, 접속한 클라이언트마다 독립된 스레드 생성
            # 클라이언트 소켓을 인자로 전달하여 handler 함수를 백그라운드에서 실행
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

    def parse_data(self, buf, is_training):
        temp = int.from_bytes(buf[0:1], byteorder="big", signed=True)
        humid = int.from_bytes(buf[1:2], byteorder="big", signed=True)
        power = int.from_bytes(buf[2:4], byteorder="big", signed=True)
        month = int.from_bytes(buf[4:5], byteorder="big", signed=True)

        lst = [temp, humid, power, month]
        logging.info("[temp, humid, power, month] = {}".format(lst))

        self.send_instance(lst, is_training)


    # 접속한 에지 디바이스와 통신하며 데이터를 처리하는 프로토콜 제어 함수
    def handler(self, client):
        logging.info("[*] Server starts to process the client's request")

        # [1단계: 학습 데이터 수집 및 반영]
        ntrain = self.ntrain
        url = "http://{}:{}/{}/training".format(self.caddr, self.cport, self.name)

        while True:
            # 클라이언트로부터 1바이트의 오프코드(명령어 종류)를 읽어옴
            # opcode (1 byte): 
            rbuf = client.recv(1)
            opcode = int.from_bytes(rbuf, "big")
            logging.debug("[*] opcode: {}".format(opcode))

            # 에지 디바이스가 데이터를 전송한 경우
            if opcode == OPCODE_DATA:
                logging.info("[*] data report from the edge")
                # 실제 데이터 본문인 5바이트를 수신
                rbuf = client.recv(5)
                logging.debug("[*] received buf: {}".format(rbuf))
                # 수신한 바이너리 데이터를 파싱하여 AI 모듈로 전송 (is_training=True)
                self.parse_data(rbuf, True)
            else:
                # 정의되지 않은 올바르지 않은 오프코드가 온 경우 에러 처리
                logging.error("[*] invalid opcode")
                logging.error("[*] please try again")
                sys.exit(1)

            # 남은 학습 데이터 개수 차감
            ntrain -= 1

            # 아직 받아야 할 학습 데이터가 남았다면 계속 보내라고 알림(OPCODE_DONE)
            if ntrain > 0:
                opcode = OPCODE_DONE
                logging.debug("[*] send the opcode OPCODE_DONE")
                client.send(int.to_bytes(opcode, 1, "big"))
            # 필요한 학습 데이터를 모두 받았다면 에지 디바이스에 대기(OPCODE_WAIT) 신호를 보내고 수집 루프 탈출
            else:
                opcode = OPCODE_WAIT
                logging.debug("[*] send the opcode OPCODE_WAIT")
                client.send(int.to_bytes(opcode, 1, "big"))
                break

        # 수집된 학습 데이터를 바탕으로 AI 모듈에 모델 학습(Training) 시작을 요청
        result = requests.post(url)
        response = json.loads(result.content)
        logging.debug("[*] return: {}".format(response["opcode"]))

        # [2단계: 테스트 데이터 수집 및 예측 수행]
        ntest = self.ntest
        url = "http://{}:{}/{}/testing".format(self.caddr, self.cport, self.name)

        # 학습이 완료되었으므로 에지 디바이스에 대기 해제 및 다음 진행 신호(OPCODE_DONE)를 전송
        opcode = OPCODE_DONE
        logging.debug("[*] send the opcode OPCODE_DONE")
        client.send(int.to_bytes(opcode, 1, "big"))

        # 필요한 테스트 데이터 개수만큼 루프 반복
        while ntest > 0:
            # 에지 디바이스로부터 1바이트 오프코드 수신
            # opcode (1 byte): 
            rbuf = client.recv(1)
            opcode = int.from_bytes(rbuf, "big")
            logging.debug("[*] opcode: {}".format(opcode))

            # 테스트 데이터가 들어온 경우
            if opcode == OPCODE_DATA:
                logging.info("[*] data report from the edge")
                # 5바이트 데이터 본문 수신
                rbuf = client.recv(5)
                logging.debug("[*] received buf: {}".format(rbuf))
                # 수신 데이터 파싱 및 AI 모듈로 테스트 데이터 전송 (is_training=False)
                self.parse_data(rbuf, False)
            else:
                logging.error("[*] invalid opcode")
                logging.error("[*] please try again")
                sys.exit(1)

            ntest -= 1
            

            # 아직 테스트 데이터가 더 남았다면 다음 데이터를 보내라고 요청(OPCODE_DONE)
            if ntest > 0:
                opcode = OPCODE_DONE
                client.send(int.to_bytes(opcode, 1, "big"))
            else:
                # 목표한 테스트 데이터를 다 받았다면 전체 통신 종료 신호(OPCODE_QUIT)를 보내고 탈출
                opcode = OPCODE_QUIT
                client.send(int.to_bytes(opcode, 1, "big"))
                break

        # [3단계: 최종 결과 요청 및 출력]
        url = "http://{}:{}/{}/result".format(self.caddr, self.cport, self.name)
        # AI 모듈로부터 최종 테스트(예측) 결과 리포트를 받아옴
        result = requests.get(url)
        response = json.loads(result.content)
        logging.debug("response: {}".format(response))
        # 결과 수신 결과 검증 및 화면 출력 처리
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
                # 최종 성공 시 결과를 포맷팅하여 콘솔에 로깅하는 함수 호출
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
    parser.add_argument("-a", "--algorithm", metavar="<AI algorithm to be used>", help="AI algorithm to be used", type=str, required=True)
    parser.add_argument("-d", "--dimension", metavar="<Dimension of each instance>", help="Dimension of each instance", type=int, default=1)
    parser.add_argument("-b", "--caddr", metavar="<AI module's IP address>", help="AI module's IP address", type=str, required=True)
    parser.add_argument("-c", "--cport", metavar="<AI module's listening port>", help="AI module's listening port", type=int, required=True)
    parser.add_argument("-p", "--lport", metavar="<server's listening port>", help="Server's listening port", type=int, required=True)
    parser.add_argument("-n", "--name", metavar="<model name>", help="Name of the model", type=str, default="model")
    parser.add_argument("-x", "--ntrain", metavar="<number of instances for training>", help="Number of instances for training", type=int, default=10)
    parser.add_argument("-y", "--ntest", metavar="<number of instances for testing>", help="Number of instances for testing", type=int, default=10)
    parser.add_argument("-z", "--index", metavar="<the index number for the power value>", help="Index number for the power value", type=int, default=0)
    parser.add_argument("-l", "--log", metavar="<log level (DEBUG/INFO/WARNING/ERROR/CRITICAL)>", help="Log level (DEBUG/INFO/WARNING/ERROR/CRITICAL)", type=str, default="INFO")
    args = parser.parse_args()
    return args

def main():
    args = command_line_args()
    logging.basicConfig(level=args.log)

    if args.ntrain <= 0 or args.ntest <= 0:
        logging.error("Number of instances for training or testing should be larger than 0")
        sys.exit(1)

    Server(args.name, args.algorithm, args.dimension, args.index, args.lport, args.caddr, args.cport, args.ntrain, args.ntest)

if __name__ == "__main__":
    main()
