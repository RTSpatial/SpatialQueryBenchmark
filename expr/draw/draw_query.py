import matplotlib.pyplot as plt
from matplotlib.scale import LogScale
import os
import numpy as np
# import comm_settings
from common import datasets,dataset_labels
import sys
import re
import pandas as pd


def get_running_time(prefix, datasets):
    loading_time = []
    query_time = []
    for dataset in datasets:
        path = os.path.join(prefix, dataset)
        with open(path, 'r') as fi:
            for line in fi:
                m = re.search(r"Loading Time (.*) ms$", line)
                if m is not None:
                    loading_time.append(float(m.groups()[0]))
                m = re.search(r"Query Time (.*) ms$", line)
                if m is not None:
                    query_time.append(float(m.groups()[0]))

    return np.asarray(loading_time), np.asarray(query_time)


patterns = ['', '\\\\', '\\\\--', '..', '..--']
light_colors = ['#6C87EA', 'lightcoral', '#FF3333', 'lemonchiffon', '#FFDF33', 'powderblue', '#33FFFF', ]
series_id = 1


def draw_build_time(prefix):
    index_types = ("rtree", "rtspatial")
    loc = [x for x in range(len(dataset_labels))]
    fig, ax = plt.subplots(nrows=1, ncols=1, figsize=(5, 4.5))

    index_loading_time = {}
    index_query_time = {}

    for index_type in index_types:
        loading_time, query_time = get_running_time(os.path.join(prefix, index_type), datasets)
        index_loading_time[index_type] = loading_time
        index_query_time[index_type] = query_time
    index_loading_time = pd.DataFrame.from_dict(index_loading_time, )

    index_loading_time.plot(kind="bar", width=0.5, ax=ax)

    ax.set_xticks(loc, dataset_labels, rotation=0)
    ax.set_xlabel(xlabel="Index Building Time")
    ax.set_ylabel(ylabel='Time (ms)', labelpad=1)
    ax.set_yscale('log')

    fig.tight_layout()
    fig.savefig(os.path.join(prefix, 'build_time.pdf'), format='pdf', bbox_inches='tight')
    plt.show()


def draw_query(prefix):
    index_types = ("rtree-parallel", "rtspatial")
    loc = [x for x in range(len(dataset_labels))]
    fig, ax = plt.subplots(nrows=1, ncols=1, figsize=(5, 4.5))

    index_loading_time = {}
    index_query_time = {}

    for index_type in index_types:
        loading_time, query_time = get_running_time(os.path.join(prefix, index_type), datasets)
        index_loading_time[index_type] = loading_time
        index_query_time[index_type] = query_time
    # index_loading_time = pd.DataFrame.from_dict(index_loading_time, orient="index", columns=list(datasets))
    # index_query_time = pd.DataFrame.from_dict(index_query_time, orient="index", columns=list(datasets))
    index_loading_time = pd.DataFrame.from_dict(index_loading_time, )
    index_query_time = pd.DataFrame.from_dict(index_query_time, )

    # for i in range(len(datasets)):
    # query_time = list(index_query_time.iloc[:, i])
    # ax = axes[i]
    # ax.bar(loc, query_time)
    # ax = axes[0]
    index_query_time.plot(kind="bar", width=0.5, ax=ax)

    ax.set_xticks(loc, dataset_labels, rotation=0)
    ax.set_xlabel("Datasets")
    ax.set_ylabel(ylabel='Query Time (ms)', labelpad=1)
    # ax.set_yscale('log')
    ax.margins(x=0.05, y=0.35)

    fig.tight_layout()
    filename = os.path.basename(prefix)
    fig.savefig(os.path.join(prefix, filename + '_point_query_time.pdf'), format='pdf', bbox_inches='tight')
    plt.show()


if __name__ == '__main__':
    dir = os.path.dirname(sys.argv[0]) + "/../query/logs"

    # draw_build_time(os.path.join(dir + "/point-contains_point-contains_queries_100000"))
    # draw_query(os.path.join(dir + "/point-contains_point-contains_queries_100000"))
    # draw_query(os.path.join(dir + "/range-contains_queries_100000"))
    draw_query(os.path.join(dir + "/range-intersects_select_0.01_queries_100000"))
