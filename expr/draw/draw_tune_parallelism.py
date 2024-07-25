import matplotlib.pyplot as plt
from matplotlib.scale import LogScale
import os
import numpy as np
from common import datasets, dataset_labels
import sys
import re
import pandas as pd
import math

patterns = ['', '\\\\', '\\\\--', '..', '..--']
light_colors = ['#6C87EA', 'lightcoral', '#FF3333', 'lemonchiffon', '#FFDF33', 'powderblue', '#33FFFF', ]
series_id = 1


def get_running_time(prefix, datasets):
    parallelism_datasets = []
    time_datasets = []
    for dataset in datasets:
        path = os.path.join(prefix, dataset)
        x = []
        y = []
        with open(path, 'r') as fi:
            for line in fi:
                line = line.strip()
                m = re.match(r"(\d+), Query Time (.*?) ms", line)
                if m is not None:
                    parallelism = int(m.groups()[0]) + 1
                    t = float(m.groups()[1])
                    x.append(parallelism)
                    y.append(t)
        parallelism_datasets.append(x)
        time_datasets.append(y)
    return np.asarray(parallelism_datasets), np.asarray(time_datasets)


def draw_query(prefix):
    datasets1 = datasets[:-1]
    parallelism_datasets, time_datasets = get_running_time(os.path.join(prefix, "rtspatial-vary-parallelism"),
                                                           datasets1)
    fig, ax = plt.subplots(nrows=1, ncols=1, figsize=(5, 4.5))

    # loc = [x for x in range(len(dataset_labels))]
    df = pd.DataFrame.from_dict(time_datasets).set_axis(datasets1)
    df = df.transpose()
    df.plot(kind="line", ax=ax)
    parallelisms = [2 ** i for i in range(len(df))]
    loc = [x for x in range(len(parallelisms))]

    ax.set_xticks(loc, parallelisms, rotation=0)
    fig.savefig(os.path.join(prefix, 'vary_parallelism.pdf'), format='pdf', bbox_inches='tight')
    plt.show()


if __name__ == '__main__':
    dir = os.path.dirname(sys.argv[0]) + "/../query/logs"
    # draw_query(os.path.join(dir + "/range-intersects_select_0.0001_queries_100000"))
    parallelisms = [2 ** i for i in range(10)]
    geoms = 3143
    queries = 100000
    hit_percent = 0.001685
    per_intersect_cost = 1

    best_n_rays = 0
    min_cost = geoms * queries
    intersect_weight = 0.999

    for n_rays in parallelisms:
        per_ray_search_cost = math.log10(queries )

        cast_rays_cost = geoms * per_ray_search_cost * n_rays
        intersect_cost = geoms * queries * hit_percent / n_rays

        cost = (1 - intersect_weight) * cast_rays_cost + intersect_weight * intersect_cost
        print(cost)
        if cost < min_cost:
            min_cost = cost
            best_n_rays = n_rays
    print("Best n rays", best_n_rays)
