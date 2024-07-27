import socket
import threading
import cv2
import numpy as np

class FaceRecognitionServer:
    def __init__(self, host='0.0.0.0', port=9000):
        self.host = host
        self.port = port
        self.recognizer = cv2.face.LBPHFaceRecognizer_create()
        self.recognizer.read('trainer/trainer.yml')
        self.faceCascade = cv2.CascadeClassifier(cv2.data.haarcascades + "haarcascade_frontalface_default.xml")
        self.font = cv2.FONT_HERSHEY_SIMPLEX
        self.cam = cv2.VideoCapture(0)
        self.cam.set(3, 640)  # Set video width
        self.cam.set(4, 480)  # Set video height
        self.minW = 0.1 * self.cam.get(3)
        self.minH = 0.1 * self.cam.get(4)
        self.fgbg = cv2.createBackgroundSubtractorMOG2()  # Object for background subtraction
        self.client_socket = None

    def receive_data(self, client_socket):
        with open("log.txt", "a") as log_file:
            while True:
                try:
                    data = client_socket.recv(256)
                    if not data:
                        print("Khách hàng đã ngắt kết nối")
                        break
                    decoded_data = data.decode('utf-8')
                    print(f"Nhận được: {decoded_data}")
                    log_file.write(decoded_data + "\n")
                    log_file.flush()
                except Exception as e:
                    print(f"Lỗi khi nhận dữ liệu: {e}")
                    break

    def send_data(self, client_socket):
        while True:
            try:
                response = input("Nhập tin nhắn để gửi: ")
                client_socket.sendall(response.encode('utf-8'))
            except Exception as e:
                print(f"Lỗi khi gửi dữ liệu: {e}")
                break

    def start_server(self):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server_socket:
            server_socket.bind((self.host, self.port))
            server_socket.listen(1)
            print(f"Máy chủ đang lắng nghe trên {self.host}:{self.port}")

            try:
                while True:
                    client_socket, client_address = server_socket.accept()
                    self.client_socket = client_socket
                    print(f"Kết nối từ {client_address}")

                    receive_thread = threading.Thread(target=self.receive_data, args=(client_socket,))
                    send_thread = threading.Thread(target=self.send_data, args=(client_socket,))
                    
                    receive_thread.start()
                    send_thread.start()

                    receive_thread.join()
                    send_thread.join()
            except KeyboardInterrupt:
                print("Máy chủ bị ngắt và đang tắt")

    def process_faces(self):
        while True:
            ret, img = self.cam.read()
            if not ret:
                print("\n[LỖI] Không thể chụp ảnh")
                break

            fgmask = self.fgbg.apply(img)
            gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
            blur = cv2.GaussianBlur(fgmask, (5, 5), 0)
            _, thresh = cv2.threshold(blur, 20, 255, cv2.THRESH_BINARY)
            dilated = cv2.dilate(thresh, None, iterations=3)
            contours, _ = cv2.findContours(dilated, cv2.RETR_TREE, cv2.CHAIN_APPROX_SIMPLE)

            for contour in contours:
                if cv2.contourArea(contour) < 900:
                    continue

                (x, y, w, h) = cv2.boundingRect(contour)
                cv2.rectangle(img, (x, y), (x + w, y + h), (0, 255, 0), 2)

                roi_gray = gray[y:y + h, x:x + w]
                faces = self.faceCascade.detectMultiScale(
                    roi_gray,
                    scaleFactor=1.2,
                    minNeighbors=5,
                    minSize=(int(self.minW), int(self.minH)),
                )

                for (fx, fy, fw, fh) in faces:
                    cv2.rectangle(img, (x + fx, y + fy), (x + fx + fw, y + fy + fh), (255, 0, 0), 2)

                    id, confidence = self.recognizer.predict(roi_gray[fy:fy + fh, fx:fx + fw])
                    
                    if confidence < 100:
                        id = f"user{id}"
                        confidence_text = f"{round(100 - confidence)}%"
                        if self.client_socket:
                            try:
                                self.client_socket.sendall('3'.encode('utf-8'))
                            except Exception as e:
                                print(f"Lỗi khi gửi dữ liệu: {e}")
                    else:
                        id = "không xác định"
                        confidence_text = f"{round(100 - confidence)}%"
                    
                    cv2.putText(img, id, (x + fx + 5, y + fy - 5), self.font, 1, (255, 255, 255), 2)
                    cv2.putText(img, confidence_text, (x + fx + 5, y + fy + fh - 5), self.font, 1, (255, 255, 0), 1)  

            cv2.imshow('camera', img)

            k = cv2.waitKey(10) & 0xff
            if k == 27:
                break

        self.cam.release()
        cv2.destroyAllWindows()

if __name__ == "__main__":
    server = FaceRecognitionServer()
    threading.Thread(target=server.start_server).start()
    server.process_faces()
