import os
import cv2 as cv
import matplotlib.pyplot as plt
import matplotlib.image as mpimg
import numpy as np
import math
import copy
from PIL import Image
from scipy.interpolate import griddata
from plotnine import *

np.set_printoptions(threshold=np.inf)


def normalize_image(img):
    norm_img = cv.normalize(img, None, alpha=0, beta=1, norm_type=cv.NORM_MINMAX, dtype=cv.CV_32F)
    return norm_img


def convert_image(img):
    new_img = []
    for item in img.getdata():
        if item[:3] == (255, 255, 255):
            new_img.append((255, 255, 255, 0))
        else:
            new_img.append(item)
    img.putdata(new_img)
    img.save('convert.png')


def get_contours(img):
    _, binary = cv.threshold(img, 0, 255, cv.THRESH_BINARY)
    contours, hierarchy = cv.findContours(binary, cv.RETR_TREE, cv.CHAIN_APPROX_SIMPLE)
    image_copy = copy.deepcopy(image)
    contour_image = cv.drawContours(image_copy, contours, -1, (0, 255, 0), 1)
    plt.imshow(np.asarray(contour_image))
    plt.axis('off')
    plt.xticks([])
    plt.yticks([])
    plt.savefig('contours.png', bbox_inches='tight', pad_inches=0.0)
    plt.show()
    contours = np.vstack(contours).squeeze()
    # print('contours', contours)
    return contours


def closest_points(point_set, p0):
    p = np.asarray(point_set)
    dist_2 = np.sum((p - p0) ** 2, axis=1)
    p_closet = np.argmin(dist_2)
    return p_closet


def get_distance(pi, pj):
    pi = np.asarray(pi)
    pj = np.asarray(pj)
    return np.linalg.norm(pi - pj)


def idw(img, point_set, p0):
    distance_list = []
    z_list = []
    p = 2
    for pi in point_set:
        dist = get_distance(pi, p0)
        if dist != 0:
            distance_list.append(dist)
            z = img[pi[1], pi[0]]
            z_list.append(z)
            t = np.power(distance_list, p)
            if t != 0:
                weight = 1 / t
                weight = list(weight)
                weight_sum = np.sum(weight)
                z_sum = np.sum(np.array(weight) * np.array(z_list))
                result = z_sum / weight_sum
                return round(result, 4)
            else:
                return 0
        else:
            return 0


if __name__ == "__main__":
    cimage = Image.open('capture.png')
    cimage = cimage.convert('RGBA')
    convert_image(cimage)

    image = cv.imread('depth_img.png')
    gray = cv.cvtColor(image, cv.COLOR_RGB2GRAY)
    gray_norm = normalize_image(gray)

    vessel_contours = get_contours(gray)

    # find void space coordinates
    black = 0
    vss_set = np.column_stack(np.where(gray == black))

    # interpolate depth in vss point
    x_list = []
    y_list = []
    z_list = []
    for point in vss_set:
        x_list.append(point[1])
        y_list.append(point[0])
        closest = closest_points(vessel_contours, point)
        cont_set = vessel_contours[closest - 3:closest + 3]
        depth = idw(gray_norm, cont_set, point)
        depth = float(depth or 0)
        # t = vessel_contours[closest]
        # depth = gray_norm[t[1], t[0]]
        print('depth is ', depth, 'at ', point)
        z_list.append(depth)

    # draw contour map
    vss_points = []
    for i in range(len(x_list)):
        point = [x_list[i], y_list[i]]
        vss_points.append(point)
    vss_points = np.array(vss_points)

    xi = np.linspace(0, 360, 51)
    yi = np.linspace(0, 200, 51)
    X, Y = np.meshgrid(xi, yi)

    Z = griddata(vss_points, z_list, (X, Y), method='cubic')

    plt.contourf(X, Y, Z, 10, cmap='Spectral')
    plt.axis('off')
    plt.xticks([])
    plt.yticks([])
    plt.savefig('vss.png', bbox_inches="tight")
    plt.show()


    # plt_img = plt.imread("capture.png")
    # fig, ax = plt.subplots()
    # ax.imshow(plt_img, extent=[0, 360, 0, 200])
    # ax.contour(X, Y, Z, cmap='bwr')
    # plt.show()
    #
    # img1 = cv.imread('convert.png')
    # img2 = cv.imread('vss.png')
    # img2 = cv.resize(img2, (360, 200))
    # dst = cv.addWeighted(img1, 0.7, img2, 0.3, 0)
    # cv.imshow('dst', dst)
    # cv.waitKey(0)
    # cv.destroyAllWindows()
