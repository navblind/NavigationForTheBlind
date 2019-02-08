import cv2
import numpy as np
from src.patterns import linearRegression, polyReg, roi, edgeDetect
from itertools import chain

cap = cv2.VideoCapture("../challenge.mp4")


# Main function
def run(img):
    vert = np.array([[100, 550], [375, 350], [450, 350], [800, 550]], np.int32)

    fin = edgeDetect(img)
    fin = roi(fin, [vert])

    cv2.imshow('fin', fin)
    line = cv2.HoughLinesP(fin, 2, np.pi / 180, 20, 7, 7)
    if line is not None:
        for i in line:
            cv2.line(img, (i[0][0], i[0][1]), (i[0][2], i[0][3]), (255, 0, 0), 10)

    ycors = [*list(line[:, 0, 1]), *list(line[:, 0, 3])]
    xcors = [*list(line[:, 0, 0]), *list(line[:, 0, 2])]

    try:

        filterl1x = []
        filterl1y = []
        filterl2x = []
        filterl2y = []

        for count, i in enumerate(ycors):
            if 400 < xcors[count]:
                filterl2x.append(xcors[count])
                filterl2y.append(i)
            else:
                filterl1x.append(xcors[count])
                filterl1y.append(i)

        try:
            equ1, polyx1, polyy1 = polyReg(filterl1x, filterl1y)
            equ2, polyx2, polyy2 = polyReg(filterl2x, filterl2y)

            for i in range(1, len(polyx1)):
                cv2.line(img, (int(polyx1[i]), int(polyy1[i])), (int(polyx1[i - 1]), int(polyy1[i - 1])),
                         (255, 255, 0), 10)
                cv2.line(img, (int(polyx2[i]), int(polyy2[i])), (int(polyx2[i - 1]), int(polyy2[i - 1])),
                         (255, 255, 0), 10)

        except Exception as e:
            print(e)

        l1m, l1b = linearRegression(filterl1x, filterl1y)
        l2m, l2b = linearRegression(filterl2x, filterl2y)

        avlm = (l1m + l2m) / 2
        avlb = (l1b + l2b) / 2

        l1inx1 = int((max(ycors) - l1b) / l1m)
        l1inx2 = int((min(ycors) - l1b) / l1m)

        l2inx1 = int((max(ycors) - l2b) / l2m)
        l2inx2 = int((min(ycors) - l2b) / l2m)

        allx = (l1inx1 + l2inx1) / 2
        ally = (l1inx2 + l2inx2) / 2

        av1 = int((max(ycors) - l1b) / l1m)
        av2 = int((max(ycors) - l2b) / l2m)

        cv2.line(img, (int(allx), max(ycors)), (int(ally), min(ycors)), (0, 255, 0), 10)

        cv2.line(img, (int(l1inx1), max(ycors)), (int(l1inx2), min(ycors)), (0, 0, 0), 10)
        cv2.line(img, (int(l2inx1), max(ycors)), (int(l2inx2), min(ycors)), (0, 0, 0), 10)

    except Exception as e:
        print(e)

    return img


# Running infinite loop to get constant video feeds
while not cv2.waitKey(1) & 0xFF == ord('q'):
    _, screen = cap.read()
    screen = cv2.resize(screen, (800, 600))

    cv2.imshow("Test", run(screen))

cap.release()
cv2.destroyAllWindows()
