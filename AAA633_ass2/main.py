import os
import cv2 as cv
import matplotlib.pyplot as plt
import numpy as np
import open3d as o3d

mesh_path = './meshes/vesselTree4.off'
path = os.path.normpath(mesh_path)


def get_mesh():
    mesh = o3d.io.read_triangle_mesh(path)
    vertices = np.asarray(mesh.vertices)
    mesh.compute_vertex_normals()
    return mesh


def custom_draw_geometry_with_key_callback(mesh):
    def set_camera(vis):
        vis.create_window()
        parameters = o3d.io.read_pinhole_camera_parameters('./assets/view_camera.json')
        vis.get_view_control().convert_from_pinhole_camera_parameters(parameters)
        return False

    def change_background_to_black(vis):
        opt = vis.get_render_option()
        opt.background_color = np.asarray([0, 0, 0])
        return False

    def capture_depth(vis):
        depth = vis.capture_depth_float_buffer()
        depth_image = (np.asarray(depth) * 100).astype(np.uint16)
        depth_image = cv.resize(depth_image, (360, 200))
        cv.imwrite('depth_img.png', depth_image)

        plt.imshow(np.asarray(depth))
        plt.show()
        return False

    def capture_image(vis):
        image = vis.capture_screen_float_buffer()
        cimage = np.asarray(image) * 255
        cimage = cv.resize(cimage, (360, 200))
        cimage = cv.cvtColor(cimage, cv.COLOR_RGB2BGR)
        cv.imwrite("capture.png", cimage)

        plt.imshow(np.asarray(image))
        plt.show()
        return False

    key_to_callback = {ord("C"): set_camera, ord("K"): change_background_to_black, ord(","): capture_depth, ord("."): capture_image}

    o3d.visualization.draw_geometries_with_key_callbacks([mesh], key_to_callback)


if __name__ == "__main__":
    vessel_mesh = get_mesh()
    custom_draw_geometry_with_key_callback(vessel_mesh)
    # s, shift + 0, Ctrl + 9
