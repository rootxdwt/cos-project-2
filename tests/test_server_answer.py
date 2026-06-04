import socket
import threading
import argparse
import logging

OPCODE_REPLY = 2

def protocol_execution(sock): # 연결된 개별 클라이언트와 데이터를 송수신하는 핵심 함수
    # 1. Alice -> Bob: length of the name (4 bytes) || name (length bytes)
    # Get the length information (4 bytes)
    buf = sock.recv(4) # 소켓 네트워크 버퍼로부터 길이 정보에 해당하는 4바이트 읽기
    length = int.from_bytes(buf, "big") # 수신된 빅엔디안 정수 바이트를 파이썬 숫자로 변환
    logging.info("[*] Length received: {}".format(length)) # 수신한 이름의 길이 값을 로그로 출력

    # Get the name (Alice)
    buf = sock.recv(length) # 위에서 파싱한 길이(length)만큼 소켓에서 이름 데이터 읽기
    logging.info("[*] Name received: {}".format(buf.decode())) # 바이트 스트림을 문자열로 디코딩하여 로그 출력

    # 2. Bob -> Alice: length of the name (4 bytes) || name (length bytes)
    # Send the length information (4 bytes)
    name = "Bob" # 서버 측 식별 이름 "Bob" 정의
    length = len(name) # 문자열 "Bob"의 길이(3) 계산
    logging.info("[*] Length to be sent: {}".format(length)) # 전송할 이름 길이 정보를 로그로 출력
    sock.send(int.to_bytes(length, 4, "big")) # 숫자를 4바이트 빅엔디안 바이너리로 변환 후 전송

    # Send the name (Bob)
    logging.info("[*] Name to be sent: {}".format(name)) # 전송할 이름 문자열을 로그로 출력
    sock.send(name.encode()) # 문자열을 바이트 스트림으로 변환(인코딩)하여 소켓 전송

    # Implement following the instructions below
    # 3. Alice -> Bob: opcode (4 bytes) || arg1 (4 bytes) || arg2 (4 bytes)
    # The opcode should be 1
    buf = sock.recv(12) # 연산 요청 패킷 규격 크기인 총 12바이트(4+4+4)를 소켓에서 수신

    # The values are encoded in the big-endian style and should be translated into the little-endian style (because my machine follows the little-endian style)
    opcode = int.from_bytes(buf[0:4], "little") # 패킷 첫 4바이트를 추출하여 연산 종류(Opcode) 복원
    arg1 = int.from_bytes(buf[4:8], "little") # 패킷 중간 4바이트를 추출하여 피연산자1(arg1) 복원
    arg2 = int.from_bytes(buf[8:12], "little") # 패킷 마지막 4바이트를 추출하여 피연산자2(arg2) 복원

    logging.info("[*] Opcode: {}".format(opcode)) # 수신된 연산 기능 분류 코드(1) 출력
    logging.info("[*] Arg1: {}".format(arg1)) # 수신된 피연산자 1 데이터 출력
    logging.info("[*] Arg2: {}".format(arg2)) # 수신된 피연산자 2 데이터 출력

    # 4. Bob -> Alice: opcode (4 bytes) || result (4 bytes)
    # The opcode should be 2
    result = arg1 + arg2 # 클라이언트가 요청한 두 정수의 더하기 연산 수행 및 결과 저장
    logging.info("[*] Result: {}".format(result)) # 최종 연산 결과 수치를 서버 로그에 기록
    opcode = 2 # (로컬 변수 opcode를 응답 규격인 2로 변경, 사용되지는 않음)
    sock.send(int.to_bytes(OPCODE_REPLY, 4, "big")) # 응답 구분값(2)을 4바이트 빅엔디안으로 소켓 송신
    sock.send(int.to_bytes(result, 4, "big")) # 계산된 결과값을 4바이트 빅엔디안으로 소켓 송신

    sock.close() # 데이터 송수신 처리가 끝난 클라이언트 소켓의 연결 자원 해제

def run(addr, port): # 서버 소켓을 개방하고 클라이언트 접속을 대기하는 구동 함수
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM) # IPv4 주소 체계와 TCP 연결형 프로토콜 소켓 생성
    server.bind((addr, port)) # 생성한 소켓에 실행 인자로 받은 IP 주소와 포트 번호 바인딩(할당)

    server.listen(2) # 연결 요청 대기 큐 크기를 2로 설정하여 클라이언트 접속 청취 시작
    logging.info("[*] Server is Listening on {}:{}".format(addr, port)) # 서버가 정상 구동되었음을 로그로 명시

    while True: # 다중 접속 수용을 위한 무한 대기 루프 실행
        client, info = server.accept() # 클라이언트 연결 요청 승인 (소켓 개체와 주소 정보 반환)

        logging.info("[*] Server accept the connection from {}:{}".format(info[0], info[1])) # 접속한 클라이언트 IP/포트 로깅

        client_handle = threading.Thread(target=protocol_execution, args=(client,)) # 비즈니스 함수 수행용 독립 스레드 생성
        client_handle.start() # 생성한 독립 스레드를 즉시 실행(백그라운드에서 백투백 처리)

def command_line_args(): # 터미널 실행 옵션을 파싱해 주는 인수 처리 함수
    parser = argparse.ArgumentParser() # 명령행 매개변수 처리를 위한 파서 객체 생성
    parser.add_argument("-a", "--addr", metavar="<server's IP address>", help="Server's IP address", type=str, default="0.0.0.0") # -a(주소) 옵션 정의 (기본값 모든 인터페이스 허용)
    parser.add_argument("-p", "--port", metavar="<server's open port>", help="Server's port", type=int, required=True) # -p(포트) 옵션 정의 (필수값 입력 지정)
    parser.add_argument("-l", "--log", metavar="<log level (DEBUG/INFO/WARNING/ERROR/CRITICAL)>", help="Log level (DEBUG/INFO/WARNING/ERROR/CRITICAL)", type=str, default="INFO") # -l(로그 레벨) 옵션 정의 (기본값 INFO)
    args = parser.parse_args() # 터미널 인자 분석 결과를 가공하여 객체 형태로 반환
    return args # 파싱 완료된 옵션 정보 반환

def main(): # 프로그램의 메인 진입 함수
    args = command_line_args() # 실행할 때 입력된 인수 파싱 결과 가져오기
    log_level = args.log # 파싱 정보에서 사용자가 지정한 로그 레벨 문자열 추출
    logging.basicConfig(level=log_level) # 추출한 레벨 기준으로 프로그램 전체 표준 로깅 환경 설정

    run(args.addr, args.port) # 실질적인 서버 바인딩 및 가동 함수 호출

if __name__ == "__main__": # 스크립트가 메인으로 직접 실행되었을 경우에만 동작 조건문
    main() # 메인 함수를 호출하여 전체 프로그램 구동
