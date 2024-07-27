#ifndef SPATIALQUERYBENCHMARK_BOOST_UPDATE_H
#define SPATIALQUERYBENCHMARK_BOOST_UPDATE_H
#include "stopwatch.h"
#include "time_stat.h"
#include "wkt_loader.h"

time_stat RunInsertionBoost(const std::vector<box_t> &boxes,
                            const BenchmarkConfig &config) {
  Stopwatch sw;
  time_stat ts;

  ts.num_geoms = boxes.size();

  boost::geometry::index::rtree<box_t,
                                boost::geometry::index::linear<BOOST_LEAF_SIZE>,
                                boost::geometry::index::indexable<box_t>>
      rtree;

  for (int i = 0; i < config.warmup + config.repeat; i++) {
    rtree.clear();
    sw.start();
    size_t n_batches = (boxes.size() + config.batch - 1) / config.batch;

    printf("n batches %lu\n", n_batches);

    for (size_t i_batch = 0; i_batch < n_batches; i_batch++) {
      auto begin = i_batch * config.batch;
      auto end = std::min(begin + config.batch, boxes.size());

      rtree.insert(boxes.begin() + begin, boxes.begin() + end);
    }
    sw.stop();
    ts.insert_ms.push_back(sw.ms());
  }

  ts.num_inserts = boxes.size();
  return ts;
}

#endif // SPATIALQUERYBENCHMARK_BOOST_UPDATE_H
