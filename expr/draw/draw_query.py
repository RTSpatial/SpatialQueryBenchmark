import matplotlib.pyplot as plt
from matplotlib.scale import LogScale
import os
import numpy as np
# import comm_settings
from common import datasets, dataset_labels, hatches, markers, linestyles
import sys
import re
import pandas as pd


def scale_size(size_list, k_scale=1000):
    return tuple(str(int(kb)) + "K" if kb < k_scale else str(int(kb / k_scale)) + "M" for kb in
                 np.asarray(size_list) / k_scale)


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


def get_running_time_vary_size(prefix, query_sizes, index_type, dataset, ):
    loading_time = []
    query_time = []
    for query_size in query_sizes:
        path = os.path.join(prefix + str(query_size), index_type, dataset)
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
    index_types = ("rtree", "rtree-parallel", "rtspatial")
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


def draw_query(prefix, index_types,
               index_labels,
               output):
    loc = [x for x in range(len(dataset_labels))]
    fig, ax = plt.subplots(nrows=1, ncols=1, figsize=(5, 4.))

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
    # 1. Choose your desired colormap
    cmap = plt.get_cmap('gist_gray')

    # 2. Segmenting the whole range (from 0 to 1) of the color map into multiple segments
    slicedCM = cmap(np.linspace(0, 1, len(index_types)))
    slicedCM = slicedCM[::-1]

    index_query_time.columns = index_labels
    bars = index_query_time.plot(kind="bar", width=0.7, ax=ax, color=slicedCM, edgecolor='black', )

    all_hatches = []
    for i in range(len(index_types)):
        all_hatches += [hatches[i] for _ in range(len(index_query_time))]

    all_hatches.reverse()
    for idx, patch in enumerate(bars.patches):
        patch.set_hatch(all_hatches[idx])

    ax.set_xticks(loc, dataset_labels, rotation=0)
    ax.set_xlabel("Datasets")
    ax.set_ylabel(ylabel='Query Time (ms)', labelpad=1)
    ax.set_yscale('log')
    ax.margins(x=0.05, y=0.35)

    ax.legend(loc='upper left', ncol=2, handletextpad=0.3,
              fontsize=11, borderaxespad=0.2, frameon=False)
    fig.tight_layout()

    fig.savefig(output, format='pdf', bbox_inches='tight')
    plt.show()


def draw_vary_query_size(prefix,
                         dataset,
                         index_types,
                         index_labels,
                         query_sizes,
                         output):
    loc = [x for x in range(len(query_sizes))]
    fig, ax = plt.subplots(nrows=1, ncols=1, figsize=(5, 4.))

    index_loading_time = {}
    index_query_time = {}

    for index_type in index_types:
        loading_time, query_time = get_running_time_vary_size(prefix, query_sizes, index_type, dataset, )
        index_loading_time[index_type] = loading_time
        index_query_time[index_type] = query_time
    index_query_time = pd.DataFrame.from_dict(index_query_time, )
    # 1. Choose your desired colormap
    cmap = plt.get_cmap('gist_gray')

    # 2. Segmenting the whole range (from 0 to 1) of the color map into multiple segments
    slicedCM = cmap(np.linspace(0, 1, len(index_types)))

    index_query_time.columns = index_labels
    index_query_time.plot(kind="line", ax=ax)
    markers = ['*', "o", '^', 's', '', 'x']
    for i, line in enumerate(ax.get_lines()):
        line.set_marker(markers[i])
        # line.set_linestyle( linestyles[])
        line.set_color('black')

    ax.set_xticks(loc, scale_size(query_sizes), rotation=0)
    ax.set_xlabel("Query Size")
    ax.set_ylabel(ylabel='Query Time (ms)', labelpad=1)
    ax.set_yscale('log')
    ax.margins(x=0.05, y=0.38)
    ax.legend(loc='upper left', ncol=3, handletextpad=0.3,
              fontsize=11, borderaxespad=0.2, frameon=False)
    fig.tight_layout()

    fig.savefig(output, format='pdf', bbox_inches='tight')
    plt.show()


def draw_range_query_intersects(prefix,
                                index_types,
                                index_labels,
                                output):
    loc = [x for x in range(len(dataset_labels))]
    fig, axes = plt.subplots(nrows=1, ncols=3, figsize=(12, 3.))

    selectivities = ("0.0001", "0.001", "0.01")

    #  "/range-intersects_select_0.0001_queries_100000"
    for idx, selectivity in enumerate(selectivities):
        ax = axes[idx]
        folder_name = "range-intersects_select_{selectivity}_queries_10000".format(selectivity=selectivity)

        index_loading_time = {}
        index_query_time = {}
        for index_type in index_types:
            loading_time, query_time = get_running_time(os.path.join(prefix, folder_name, index_type), datasets)
            index_loading_time[index_type] = loading_time
            index_query_time[index_type] = query_time
        index_query_time = pd.DataFrame.from_dict(index_query_time, )

        # 1. Choose your desired colormap
        cmap = plt.get_cmap('gist_gray')

        # 2. Segmenting the whole range (from 0 to 1) of the color map into multiple segments
        slicedCM = cmap(np.linspace(0, 1, len(index_types)))
        slicedCM = slicedCM[::-1]

        index_query_time.columns = index_labels
        bars = index_query_time.plot(kind="bar", width=0.5, ax=ax, color=slicedCM, edgecolor='black', )

        all_hatches = []
        for i in range(len(index_types)):
            all_hatches += [hatches[i] for _ in range(len(index_query_time))]

        all_hatches.reverse()
        for idx, patch in enumerate(bars.patches):
            patch.set_hatch(all_hatches[idx])

        ax.set_xticks(loc, dataset_labels, rotation=0)
        ax.set_xlabel(str(float(selectivity) * 100) + "% Selectivity")
        ax.set_ylabel(ylabel='Query Time (ms)', labelpad=1)
        ax.set_yscale('log')
        ax.margins(x=0.05, y=0.35)

        ax.legend(loc='upper left', ncol=2, handletextpad=0.3,
                  fontsize=11, borderaxespad=0.2, frameon=False)
    fig.tight_layout()

    fig.savefig(output, format='pdf', bbox_inches='tight')
    plt.show()


if __name__ == '__main__':
    dir = os.path.dirname(sys.argv[0]) + "/../query/logs"

    # draw_query(os.path.join(dir + "/point-contains_point-contains_queries_100000"),
    #            ("rtree", "cgal", "cuspatial", "lbvh", "rtspatial"),
    #            ("Boost", "CGAL", "cuSpatial", "LBVH", "RTSpatial"),
    #            'point_query_time.pdf')

    # draw_vary_query_size(os.path.join(dir + "/point-contains_point-contains_queries_"),
    #                      "parks_Europe.wkt.log",
    #                      ("rtree", "cgal", "cuspatial", "lbvh", "rtspatial"),
    #                      ("Boost", "CGAL", "cuSpatial", "LBVH", "RTSpatial"),
    #                      (50000, 100000, 200000, 400000, 800000,),
    #                      'point_query_time_vary_size.pdf')

    # draw_query(os.path.join(dir + "/range-contains_queries_100000"),
    #            ("rtree", "glin","lbvh", "rtspatial"),
    #            ("Boost", "GLIN", "LBVH", "RTSpatial"),
    #            'range_query_contains_time.pdf')

    draw_range_query_intersects(os.path.join(dir),
                                ("rtree", "glin", "lbvh", "rtspatial"),
                                ("Boost", "GLIN", "LBVH", "RTSpatial"),
                                'range_query_intersects_time.pdf')

    # draw_query(os.path.join(dir + "/range-contains_queries_100000"),
    #            ("rtree", "rtree-parallel", "lbvh", "rtspatial"),
    #            ("Boost", "Boost Parallel", "LBVH", "RTSpatial"),
    #            'range_contains_query_time.pdf')

    # draw_query(os.path.join(dir + "/range-intersects_select_0.01_queries_100000"),
    #            ("rtree-parallel", "lbvh", "rtspatial"),
    #            ("Boost Parallel", "LBVH", "RTSpatial"),
    #            'range_intersects_query_time.pdf')
    # draw_query(os.path.join(dir + "/range-contains_queries_100000"))
    # draw_query(os.path.join(dir + "/range-intersects_select_0.01_queries_100000"))
