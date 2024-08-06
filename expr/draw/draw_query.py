import matplotlib
import matplotlib.pyplot as plt
import math
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


def get_running_time_vary_parallelism(prefix, datasets):
    dataset_query_time = {}
    predicated_parallelism = []
    predicate_time = []
    forward_time = []
    bvh_time = []
    backward_time = []

    for dataset in datasets:
        path = os.path.join(prefix, dataset)
        query_time = []
        with open(path, 'r') as fi:
            for line in fi:
                m = re.search(r"Predicated Parallelism (\d+) Time (.*?) ms", line)
                if m is not None:
                    predicated_parallelism.append(int(m.groups()[0]))
                    predicate_time.append(float(m.groups()[1]))
                m = re.search(r"Query Time (.*) ms$", line)
                if m is not None:
                    query_time.append(float(m.groups()[0]))
                m = re.search(r"Forward pass (.*?) ms, results \d+ BVH (.*?) ms, Backward pass (.*?) ms, results \d+",
                              line)
                if m is not None:
                    forward_time.append(float(m.groups()[0]))
                    bvh_time.append(float(m.groups()[1]))
                    backward_time.append(float(m.groups()[2]))
        dataset_query_time[dataset] = query_time

    predicate_time = np.asarray(predicate_time)
    forward_time = np.asarray(forward_time)
    bvh_time = np.asarray(bvh_time)
    backward_time = np.asarray(backward_time)
    total_time = predicate_time + forward_time + bvh_time + backward_time

    time_breakdown = pd.DataFrame({"Predication": predicate_time / total_time * 100,
                                   "Forward Pass": forward_time / total_time * 100,
                                   "BVH Buildup": bvh_time / total_time * 100,
                                   "Backward Pass": backward_time / total_time * 100})

    return predicated_parallelism, dataset_query_time, time_breakdown


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
    # plt.show()


def draw_point_query(prefix, ):
    index_types = ("rtree", "cgal", "cuspatial", "lbvh", "rtspatial")
    index_labels = ("Boost", "CGAL", "cuSpatial", "LBVH", "RTSpatial")

    plt.rcParams.update({'font.size': 14})
    loc = [x for x in range(len(dataset_labels))]
    fig, axes = plt.subplots(nrows=1, ncols=2, figsize=(11, 3.6))
    fig.subplots_adjust(wspace=0.05)  # Adjust the width space between axes

    ax1 = axes[0]
    ax2 = axes[1]

    # Varying datasets

    index_loading_time = {}
    index_query_time = {}
    query_size = 100000

    for index_type in index_types:
        loading_time, query_time = get_running_time(
            os.path.join(prefix + "/point-contains_queries_" + str(query_size), index_type), datasets)
        index_loading_time[index_type] = loading_time
        index_query_time[index_type] = query_time
    index_query_time = pd.DataFrame.from_dict(index_query_time, )

    print("Point Contains Summaries")

    for i in range(len(datasets)):
        rtree_time = index_query_time.iloc[i][0]
        cgal_time = index_query_time.iloc[i][1]
        cpu_time = min(rtree_time, cgal_time)
        cuspatial_time = index_query_time.iloc[i][2]
        lbvh_time = index_query_time.iloc[i][3]
        gpu_time = min(cuspatial_time, lbvh_time)
        rtspatial_time = index_query_time.iloc[i][4]
        print("Dataset", datasets[i])
        print("Speedup over best CPU", cpu_time / rtspatial_time)
        print("Speedup over best GPU", gpu_time / rtspatial_time)
        print()

    # 1. Choose your desired colormap
    cmap = plt.get_cmap('gist_gray')

    # 2. Segmenting the whole range (from 0 to 1) of the color map into multiple segments
    slicedCM = cmap(np.linspace(0, 1, len(index_types)))
    slicedCM = slicedCM[::-1]

    index_query_time.columns = index_labels
    index_query_time.plot(kind="bar", width=0.85, ax=ax1, color=slicedCM, edgecolor='black', )

    bars = [thing for thing in ax1.containers if isinstance(thing, matplotlib.container.BarContainer)]
    for i, bar in zip(range(len(bars)), bars):
        for patch in bar:
            patch.set_hatch(hatches[i])

    ax1.set_xticks(loc, dataset_labels, rotation=0)
    ax1.set_xlabel("(a) Execution Time on 100K Point Queries")
    ax1.set_ylabel(ylabel='Query Time (ms)', labelpad=1)
    ax1.set_yscale('log')
    x0, x1 = ax1.get_xlim()
    ax1.set_xlim(x0 + 0.15, x1 - 0.15)  # x-margins does not work with pandas
    ax1.margins(y=0.2)
    ax1.legend(loc='upper left', ncol=3, handletextpad=0.3,
               borderaxespad=0.2, columnspacing=1, frameon=False,
               ) # bbox_to_anchor=(-0.02, 1.25)

    # Varying sizes
    query_sizes = (50000, 100000, 200000, 400000, 800000,)

    index_loading_time = {}
    index_query_time = {}
    dataset = "parks_Europe.wkt.log"

    for index_type in index_types:
        loading_time, query_time = get_running_time_vary_size(prefix + "/point-contains_queries_",
                                                              query_sizes, index_type, dataset, )
        index_loading_time[index_type] = loading_time
        index_query_time[index_type] = query_time
    index_query_time = pd.DataFrame.from_dict(index_query_time, )
    index_query_time.columns = index_labels
    index_query_time.plot(kind="line", ax=ax2)

    for i, line in enumerate(ax2.get_lines()):
        line.set_marker(markers[i])
        line.set_color('black')

    loc = [x for x in range(len(query_sizes))]
    ax2.set_xticks(loc, scale_size(query_sizes), rotation=0)
    ax2.set_xlabel("(b) Varying query size on EUParks dataset")
    ax2.set_ylabel(ylabel='Query Time (ms)', labelpad=1)
    ax2.set_yscale('log')
    ax2.margins(y=0.38)
    ax2.legend(loc='upper left', ncol=3, handletextpad=0.3,
               borderaxespad=0.2, columnspacing=1, frameon=False,
               ) # bbox_to_anchor=(-0.02, 1.25)

    fig.tight_layout(pad=0.)

    fig.savefig("point_query.pdf", format='pdf', bbox_inches='tight')
    plt.show()

def draw_range_contains_query(prefix, ):
    index_types = ("rtree", "glin", "lbvh", "rtspatial")
    index_labels = ("Boost", "GLIN", "LBVH", "RTSpatial")

    plt.rcParams.update({'font.size': 14})
    loc = [x for x in range(len(dataset_labels))]
    fig, axes = plt.subplots(nrows=1, ncols=2, figsize=(10.5, 3.6))
    fig.subplots_adjust(wspace=1)  # Adjust the width space between axes

    ax1 = axes[0]
    ax2 = axes[1]

    # Varying datasets

    index_loading_time = {}
    index_query_time = {}
    query_size = 100000

    for index_type in index_types:
        loading_time, query_time = get_running_time(
            os.path.join(prefix + "/range-contains_queries_" + str(query_size), index_type), datasets)
        index_loading_time[index_type] = loading_time
        index_query_time[index_type] = query_time
    index_query_time = pd.DataFrame.from_dict(index_query_time, )

    print("Range Contains Summaries")

    for i in range(len(datasets)):
        rtree_time = index_query_time.iloc[i][0]
        glin_time = index_query_time.iloc[i][1]
        cpu_time = min(rtree_time, glin_time)
        lbvh_time = index_query_time.iloc[i][2]
        rtspatial_time = index_query_time.iloc[i][3]
        print("Dataset", datasets[i])
        print("Speedup over best CPU", cpu_time / rtspatial_time)
        print("Speedup over best GPU", lbvh_time / rtspatial_time)
        print()

    # 1. Choose your desired colormap
    cmap = plt.get_cmap('gist_gray')

    # 2. Segmenting the whole range (from 0 to 1) of the color map into multiple segments
    slicedCM = cmap(np.linspace(0, 1, len(index_types)))
    slicedCM = slicedCM[::-1]

    index_query_time.columns = index_labels
    index_query_time.plot(kind="bar", width=0.8, ax=ax1, color=slicedCM, edgecolor='black', )

    bars = [thing for thing in ax1.containers if isinstance(thing, matplotlib.container.BarContainer)]
    for i, bar in zip(range(len(bars)), bars):
        for patch in bar:
            patch.set_hatch(hatches[i])

    ax1.set_xticks(loc, dataset_labels, rotation=0)
    ax1.set_xlabel("(a) Execution time on 100K range-contains queries")
    ax1.set_ylabel(ylabel='Query Time (ms)', labelpad=1)
    ax1.set_yscale('log')
    x0, x1 = ax1.get_xlim()
    ax1.set_xlim(x0 + 0.15, x1 - 0.15)  # x-margins does not work with pandas
    ax1.margins(y=0.4)

    ax1.legend(loc='upper left', ncol=2, handletextpad=0.3,
               borderaxespad=0.2, columnspacing=1, frameon=False,
               )

    # Varying sizes
    query_sizes = (50000, 100000, 200000, 400000, 800000,)

    index_loading_time = {}
    index_query_time = {}
    dataset = "parks_Europe.wkt.log"

    for index_type in index_types:
        loading_time, query_time = get_running_time_vary_size(prefix + "/range-contains_queries_",
                                                              query_sizes, index_type, dataset, )
        index_loading_time[index_type] = loading_time
        index_query_time[index_type] = query_time
    index_query_time = pd.DataFrame.from_dict(index_query_time, )
    index_query_time.columns = index_labels
    index_query_time.plot(kind="line", ax=ax2)
    markers = ['*', "o", '^', '', 's', 'x']

    for i, line in enumerate(ax2.get_lines()):
        line.set_marker(markers[i])
        # line.set_linestyle( linestyles[])
        line.set_color('black')

    loc = [x for x in range(len(query_sizes))]
    ax2.set_xticks(loc, scale_size(query_sizes), rotation=0)
    ax2.set_xlabel("(b) Varying query size on EUParks dataset")
    ax2.set_ylabel(ylabel='Query Time (ms)', labelpad=1)
    ax2.set_yscale('log')
    ax2.margins(y=0.4)
    ax2.legend(loc='upper left', ncol=2, handletextpad=0.3,
               borderaxespad=0.2, frameon=False,
               )

    fig.tight_layout(pad=0.1)

    fig.savefig("range_contains_query.pdf", format='pdf', bbox_inches='tight')
    plt.show()


def draw_range_query_intersects(prefix,
                                index_types,
                                index_labels,
                                ):
    loc = [x for x in range(len(dataset_labels))]
    plt.rcParams.update({'font.size': 11})
    fig, axes = plt.subplots(nrows=1, ncols=3, figsize=(12, 3.))
    # fig.subplots_adjust(hspace=0.01)  # Adjust the height space between axes
    fig.subplots_adjust(wspace=0.6)  # Adjust the width space between axes

    fig_names = ("(a)", "(b)", "(c)")
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

        print("Range Intersects, Selectivity ", selectivity)

        for i in range(len(datasets)):
            rtree_time = index_query_time.iloc[i][0]
            glin_time = index_query_time.iloc[i][1]
            cpu_time = min(rtree_time, glin_time)
            lbvh_time = index_query_time.iloc[i][2]
            rtspatial_time = index_query_time.iloc[i][3]
            print("Dataset", datasets[i])
            print("Speedup over best CPU", cpu_time / rtspatial_time)
            print("Speedup over best GPU", lbvh_time / rtspatial_time)
            print("Speedup over the best", min(cpu_time, lbvh_time) / rtspatial_time)
            print()

        # 1. Choose your desired colormap
        cmap = plt.get_cmap('gist_gray')

        # 2. Segmenting the whole range (from 0 to 1) of the color map into multiple segments
        slicedCM = cmap(np.linspace(0, 1, len(index_types)))
        slicedCM = slicedCM[::-1]

        index_query_time.columns = index_labels
        index_query_time.plot(kind="bar", width=0.8, ax=ax, color=slicedCM, edgecolor='black', )

        bars = [thing for thing in ax.containers if isinstance(thing, matplotlib.container.BarContainer)]
        for i, bar in zip(range(len(bars)), bars):
            for patch in bar:
                patch.set_hatch(hatches[i])

        ax.set_xticks(loc, dataset_labels, rotation=0)
        ax.set_xlabel(fig_names[idx] + " " + str(float(selectivity) * 100) + "% Selectivity")
        ax.set_ylabel(ylabel='Query Time (ms)', labelpad=1)
        ax.set_yscale('log')
        ax.margins(x=0.05, y=0.1)
        x0, x1 = ax.get_xlim()
        ax.set_xlim(x0 + 0.15, x1 - 0.15)  # x-margins does not work with pandas
        ax.legend(loc='upper left', ncol=2, handletextpad=0.3,
                  borderaxespad=0.2, frameon=False)
    fig.tight_layout(pad=0.1)

    fig.savefig('range_intersects_query.pdf', format='pdf', bbox_inches='tight')
    plt.show()


def draw_vary_rays(prefix):
    plt.rcParams.update({'font.size': 14})
    fig, axes = plt.subplots(nrows=1, ncols=2, figsize=(10, 4.5,))
    fig.subplots_adjust(wspace=-0.12)  # Adjust the width space between axes

    ax1 = axes[0]
    ax2 = axes[1]

    index_type = "rtspatial-vary-parallelism"

    predicated_parallelism, dataset_query_time, time_breakdown = get_running_time_vary_parallelism(
        os.path.join(prefix + "/ray_duplication_range-intersects_select_0.0001_queries_50000", index_type), datasets)
    dataset_query_time = pd.DataFrame.from_dict(dataset_query_time, )
    # 1. Choose your desired colormap
    cmap = plt.get_cmap('gist_gray')

    # 2. Segmenting the whole range (from 0 to 1) of the color map into multiple segments
    slicedCM = cmap(np.linspace(0, 1, len(datasets) + 1))

    dataset_query_time.columns = dataset_labels
    dataset_query_time.plot(kind="line", ax=ax1, color=slicedCM, )

    for i, line in enumerate(ax1.get_lines()):
        line.set_marker(markers[i])
        line.set_color('black')

    loc = [x for x in range(len(dataset_query_time))]
    x_labels = [2 ** i for i in range(len(loc))]

    row_ids = [int(math.log2(p)) for p in predicated_parallelism]

    for i in range(len(datasets)):
        x = row_ids[i]
        y = dataset_query_time.iloc[row_ids[i]][i]
        label = ""
        if i == 0:
            label = "Predication"
        ax1.plot(x, y, 'ro', label=label, markersize=12, mfc='none')

    ax1.set_xticks(loc, x_labels, rotation=0)
    ax1.set_xlabel("(a) Number of duplicated rays per query")
    ax1.set_ylabel(ylabel='Query Time (ms)', labelpad=1)
    ax1.set_yscale('log')
    ax1.margins(x=0.05, y=0.1)

    ax1.legend(loc='upper left', ncol=3, handletextpad=0.3,
               borderaxespad=0.2, columnspacing=0.5, frameon=False,
               bbox_to_anchor=(-0.08, 1.2))

    time_breakdown.plot(kind="bar", stacked=True, ax=ax2, color=slicedCM)
    # bars = [thing for thing in ax2.containers if isinstance(thing, matplotlib.container.BarContainer)]

    # patterns = ('', '//', 'xx', r'\\', '\\', '*', 'o', 'O', '.')
    # for i, bar in zip(range(len(bars)), bars):
    #     for patch in bar:
    #         patch.set_hatch(patterns[i])

    loc = [x for x in range(len(dataset_labels))]
    ax2.set_xticks(loc, dataset_labels, rotation=0)
    ax2.set_xlabel("(b) Running time breakdown")
    ax2.set_ylabel(ylabel='Time Percentage (%)', labelpad=1)
    bars = [thing for thing in ax2.containers if isinstance(thing, matplotlib.container.BarContainer)]
    hatches = ['--', '//', 'xx',  '\\\\', '']
    for i, bar in zip(range(len(bars)), bars):
        for patch in bar:
            patch.set_hatch(hatches[i])
    # handletextpad=0.3,
    # borderaxespad=0.2, frameon=False,
    legend = ax2.legend(loc='upper left', ncol=2,
                        columnspacing=1, frameon=False,
                        bbox_to_anchor=(-0.05, 1.22))
    legend.get_frame().set_alpha(0.5)
    fig.tight_layout(pad=0.1)

    fig.savefig("dup_rays.pdf", format='pdf', bbox_inches='tight')
    plt.show()


if __name__ == '__main__':
    dir = os.path.dirname(sys.argv[0]) + "/../query/logs"

    # draw_point_query(dir)

    # draw_range_contains_query(dir)

    draw_range_query_intersects(os.path.join(dir),
                                ("rtree", "glin", "lbvh", "rtspatial"),
                                ("Boost", "GLIN", "LBVH", "RTSpatial"),
                                )

    # draw_vary_rays(dir)
