```python
    file = open('vss_list.txt', 'w')
    for items in vss_list:
        file.writelines(items + '\n')
    file.close()
```

```python

    vss_list = []
    vss_distance = []
    for vss_point in vss_coords:

        for con_point in contours:
            if con_point[0] == vss_point[0] and con_point[1] == vss_point[1]:
                depth = img[con_point[1], con_point[0]]

            else:
                # get distance to each contour and save in list[con_x, con_y, distance]
                distance = get_distance(vss_point, con_point)
                z = copy.deepcopy(con_point)
                z = z.tolist()
                z.append(distance)
                vss_distance.append(z)
                vss_distance = sorted(vss_distance, key=lambda x: x[2])

                # inverse distance weighting to interpolate depth in vss point
                for point in vss_distance[0:10]:
                    weight = 1 / math.pow(point[2], p)
                    sum_depth += weight * img[point[1], point[0]]
                    sum_weight += weight
                depth = sum_depth / sum_weight

        print('depth is ', depth, ' at ', vss_point)

        d = copy.deepcopy(vss_point)
        d = d.tolist()
        d.append(depth)
        vss_list.append(d)
```

```python
[297, 203, 0.36986300349235535]
```

