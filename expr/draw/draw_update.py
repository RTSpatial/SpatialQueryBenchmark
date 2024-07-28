import matplotlib
import matplotlib.pyplot as plt
import math
import os
import numpy as np
# import comm_settings
from common import hatches, markers, linestyles
import sys
import re
import pandas as pd


def scale_size(size_list, k_scale=1000):
    return tuple(str(int(kb)) + "K" if kb < k_scale else str(int(kb / k_scale)) + "M" for kb in
                 np.asarray(size_list) / k_scale)


def get_update_time(prefix, op, dataset):
    inserted_list = []
    time_list = []
    path = os.path.join(prefix, op, dataset)
    with open(path, 'r') as fi:
        for line in fi:
            m = re.search(r"Geoms (\d+) .*? Time (.*?) ms", line)
            if m is not None:
                n_geoms = int(m.groups()[0]) / 1000 / 1000
                inserted_list.append(n_geoms)
                time_list.append(float(m.groups()[1]))

    return np.asarray(inserted_list), np.asarray(time_list)


def get_update_throughput(prefix, op, dataset, batch_sizes):
    throughput_list = []
    for batch in batch_sizes:
        path = os.path.join(prefix, op + "_batch_" + batch, dataset)
        with open(path, 'r') as fi:
            for line in fi:
                m = re.search(r"Throughput (.*?) ge", line)
                if m is not None:
                    throughput = float(m.groups()[0]) / 1000 / 1000
                    throughput_list.append(throughput)

    return np.asarray(throughput_list)


def get_query_performance_slowdown(prefix, op, dataset, query_size, update_ratios):
    slowdown_list = []
    for ratio in update_ratios:
        path = os.path.join(prefix, op + "_update_" + ratio + "_queries_" + query_size, dataset)
        with open(path, 'r') as fi:
            query_time = None
            query_time_updated = None
            for line in fi:
                m = re.search(r"Query Time (\d.*?) ms", line)
                if m is not None:
                    query_time = float(m.groups()[0])
                m = re.search(r"Query Time After Updates (.*?) ms", line)
                if m is not None:
                    query_time_updated = float(m.groups()[0])
            slowdown_list.append(query_time_updated / query_time)
    return np.asarray(slowdown_list)


patterns = ['', '\\\\', '\\\\--', '..', '..--']
light_colors = ['#6C87EA', 'lightcoral', '#FF3333', 'lemonchiffon', '#FFDF33', 'powderblue', '#33FFFF', ]
series_id = 1


def scale_size(size_list, k_scale=1000):
    return tuple(str(int(kb)) + "K" if kb < k_scale else str(int(kb / k_scale)) + "M" for kb in
                 np.asarray(size_list) / k_scale)


def draw_bulk_time(prefix):
    fig, ax = plt.subplots(nrows=1, ncols=1, figsize=(5, 4.5))
    cmap = plt.get_cmap('gist_gray')
    slicedCM = cmap(np.linspace(0, 1, 5))

    inserted_list, time_list = get_update_time(prefix, "insertion", "uniform_n_50000000.wkt.log")
    ax.plot(inserted_list, time_list, label="Insertion", color=slicedCM[3])
    inserted_list, time_list = get_update_time(prefix, "deletion", "uniform_n_50000000.wkt.log")
    ax.plot(inserted_list, time_list, label="Deletion", color=slicedCM[0])

    # ax.set_xticks(loc, dataset_labels, rotation=0)
    ax.set_xlabel(xlabel="Millions of Geometries")
    ax.set_ylabel(ylabel='Time (ms)', labelpad=1)
    ax.legend(loc='upper left',
              ncol=1, handletextpad=0.3,
              fontsize=11, borderaxespad=0.2, frameon=False)

    fig.tight_layout()
    fig.savefig('ins_del.pdf', format='pdf', bbox_inches='tight')
    plt.show()


def draw_batch_throughput(prefix):
    fig, ax = plt.subplots(nrows=1, ncols=1, figsize=(5, 4.5))
    cmap = plt.get_cmap('gist_gray')
    batch_sizes = ("1000", "10000", "100000", "1000000",)

    insert_tp = get_update_throughput(prefix, "insertion", "uniform_n_50000000.wkt.log", batch_sizes)
    delete_tp = get_update_throughput(prefix, "deletion", "uniform_n_50000000.wkt.log", batch_sizes)

    df = pd.DataFrame.from_dict({"Insertion": insert_tp, "Deletion": delete_tp}, )
    df.index = scale_size([int(n) for n in np.asarray(batch_sizes, dtype=int)])

    slicedCM = cmap(np.linspace(0, 1, 2))
    slicedCM = slicedCM[::-1]
    df.plot(kind="bar", ax=ax, rot=0, color=slicedCM, edgecolor='black', )

    ax.set_xlabel(xlabel="Batch Size")
    ax.set_ylabel(ylabel='Throughput (M geometries/sec)', labelpad=1)
    ax.set_yscale('log')
    ax.legend(loc='upper left',
              ncol=1, handletextpad=0.3,
              fontsize=11, borderaxespad=0.2, frameon=False)

    fig.tight_layout()
    fig.savefig('ins_del_tp.pdf', format='pdf', bbox_inches='tight')
    plt.show()


def draw_update_query(prefix):
    fig, ax = plt.subplots(nrows=1, ncols=1, figsize=(5, 4.5))
    cmap = plt.get_cmap('gist_gray')
    dataset = "parks_Europe.wkt.log"
    update_ratios = ("0.0002", "0.002", "0.02", "0.2",)

    point_contains_slowdown = get_query_performance_slowdown(prefix, "point-contains", dataset, "100000", update_ratios)
    range_contains_slowdown = get_query_performance_slowdown(prefix, "range-contains", dataset, "100000", update_ratios)
    range_intersects_slowdown = get_query_performance_slowdown(prefix, "range-intersects", dataset, "10000",
                                                               update_ratios)

    df = pd.DataFrame.from_dict(
        {"Point Query": point_contains_slowdown, "Range Contains Query": range_contains_slowdown,
         "Range Intersects Query": range_intersects_slowdown}, )
    df.index = [str(float(r) * 100) + "%" for r in update_ratios]

    slicedCM = cmap(np.linspace(0, 1, 3))
    slicedCM = slicedCM[::-1]
    df.plot(kind="bar", ax=ax, rot=0, color=slicedCM, edgecolor='black', )

    ax.margins(x=0.05, y=0.38)
    ax.set_xlabel(xlabel="Update Ratio")
    ax.set_ylabel(ylabel='Query Performance Slowdown', labelpad=1)
    ax.legend(loc='upper left',
              ncol=1, handletextpad=0.3,
              fontsize=11, borderaxespad=0.2, frameon=False)

    fig.tight_layout()
    fig.savefig('update_query.pdf', format='pdf', bbox_inches='tight')
    # plt.show()


if __name__ == '__main__':
    dir = os.path.dirname(sys.argv[0]) + "/../query/logs"

    # draw_bulk_time(dir)
    # draw_batch_throughput(dir)
    draw_update_query(dir)
