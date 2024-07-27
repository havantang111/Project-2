import cv2
import numpy as np
from PIL import Image
import os


current_dir = os.getcwd()
dataset_dir = os.path.join(current_dir, 'dataset')

if not os.path.exists(dataset_dir):
    os.makedirs(dataset_dir)

cam = cv2.VideoCapture(0)
cam.set(3, 640)  
cam.set(4, 480)  

face_cascade = cv2.CascadeClassifier(cv2.data.haarcascades + 'haarcascade_frontalface_default.xml')

face_id = input('\nEnter user ID and press Enter: ')

print("\n[INFO] Initializing face capture. Look at the camera and wait...")


count = 0

while True:
    
    ret, img = cam.read()
    if not ret:
        print("\n[ERROR] Failed to capture image")
        break

    img = cv2.flip(img, 1)  
    gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)  

    
    faces = face_cascade.detectMultiScale(gray, scaleFactor=1.3, minNeighbors=5)

    for (x, y, w, h) in faces:
        
        cv2.rectangle(img, (x, y), (x + w, y + h), (255, 0, 0), 2)
        count += 1

        
        face_img = gray[y:y + h, x:x + w]
        file_path = os.path.join(dataset_dir, f"User.{face_id}.{count}.jpg")
        cv2.imwrite(file_path, face_img)

    
    cv2.imshow('image', img)

    
    k = cv2.waitKey(100) & 0xff
    if k == 27:  
        break
    elif count >= 200:  
        break

print("\n[INFO] Exiting Program and cleaning up...")
cam.release()
cv2.destroyAllWindows()

trainer_dir = 'trainer'
if not os.path.exists(trainer_dir):
    os.makedirs(trainer_dir)

# Khởi tạo đối tượng nhận diện khuôn mặt và bộ phân loại Haar Cascade
recognizer = cv2.face.LBPHFaceRecognizer_create()
detector = cv2.CascadeClassifier(cv2.data.haarcascades + 'haarcascade_frontalface_default.xml')

def getImagesAndLabels(path):
    imagePaths = [os.path.join(path, f) for f in os.listdir(path)]     
    faceSamples = []
    ids = []

    for imagePath in imagePaths:
        
        PIL_img = Image.open(imagePath).convert('L') 
        img_numpy = np.array(PIL_img, 'uint8')

       
        try:
            id = int(os.path.split(imagePath)[-1].split(".")[1])
        except ValueError:
            print(f"ID không hợp lệ từ tệp {imagePath}. Bỏ qua.")
            continue

        
        faces = detector.detectMultiScale(img_numpy)

        for (x, y, w, h) in faces:
            faceSamples.append(img_numpy[y:y + h, x:x + w])
            ids.append(id)

    return faceSamples, ids

print("\n[INFO] Training faces. It will take a few seconds. Wait ...")
faces, ids = getImagesAndLabels(dataset_dir)
if len(faces) == 0:
    print("[ERROR] Không có khuôn mặt nào được tìm thấy để huấn luyện.")
else:
    recognizer.train(faces, np.array(ids))
    recognizer.save(os.path.join(trainer_dir, 'trainer.yml')) 

    print("\n[INFO] {0} faces trained. Exiting Program".format(len(np.unique(ids))))
