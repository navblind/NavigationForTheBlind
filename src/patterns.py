import math
from scipy.optimize import curve_fit
import numpy as np
import cv2

def linearRegression(X, Y):
    mean_x = np.mean(X)
    mean_y = np.mean(Y)
    m = len(X)
    numer = 0
    denom = 0
    for i in range(m):
        numer += (X[i] - mean_x) * (Y[i] - mean_y)
        denom += (X[i] - mean_x) ** 2
    b1 = numer / denom
    b0 = mean_y - (b1 * mean_x)

    max_x = np.max(X) + 5
    min_x = np.min(X) - 5
    x = np.linspace(min_x, max_x, 1000)
    y = b0 + b1 * x

    return b1, b0


def polyReg(xcors, ycors):
    def func(x, a, b, c):
        return (a*(x**2)) + (b*x) + c
    time = np.array(xcors)
    avg = np.array(ycors)
    initialGuess = [5, 5, -.01]
    guessedFactors = [func(x, *initialGuess) for x in time]
    popt, pcov = curve_fit(func, time, avg, initialGuess)
    cont = np.linspace(min(time), max(time), 50)
    fittedData = [func(x, *popt) for x in cont]

    xcors = []
    ycors = []
    for count, i in enumerate(cont):
       xcors.append(i)
       ycors.append(fittedData[count])

    return popt, xcors, ycors

#Cutting out the reigon of interest
def roi(img, vert):
    mask = np.zeros_like(img)
    cv2.fillPoly(mask, vert, 255)
    return cv2.bitwise_and(img, mask)


#Performing edge detection
def edgeDetect(img):
     edges = cv2.Canny(img, 250, 300)
     return cv2.GaussianBlur(edges, (5,5), 0)
