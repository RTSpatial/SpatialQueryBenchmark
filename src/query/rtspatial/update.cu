#include "query/rtspatial/common.h"
#include "query/rtspatial/update.h"
#include "rtspatial/rtspatial.h"
#include "stopwatch.h"


time_stat RunInsertionRTSpatial(const std::vector<box_t> &boxes,
                                const BenchmarkConfig &config) {
  rtspatial::Stream stream;
  rtspatial::SpatialIndex<coord_t, 2> index;
  thrust::device_vector<rtspatial::Envelope<rtspatial::Point<coord_t, 2>>>
      d_boxes;
  rtspatial::Config idx_config;

  idx_config.ptx_root = std::string(RTSPATIAL_PTX_DIR);
  idx_config.max_geometries = boxes.size();
  CopyBoxes(boxes, d_boxes);

  index.Init(idx_config);
  time_stat ts;
  Stopwatch sw;

  ts.num_geoms = boxes.size();

  int batch = config.batch;

  if (batch == -1) {
    size_t n_steps = 100;
    size_t avg_gemos_per_step = (boxes.size() + n_steps - 1) / n_steps;
    size_t n_inserted = 0;

    for (size_t i = 0; i < n_steps; i++) {
      auto begin = i * avg_gemos_per_step;
      auto size = std::min(begin + avg_gemos_per_step, boxes.size()) - begin;
      double total_insert_time = 0;

      n_inserted += size;

      for (int repeat = 0; repeat < config.repeat; repeat++) {
        index.Clear();

        sw.start();
        index.Insert(rtspatial::ArrayView<
                         rtspatial::Envelope<rtspatial::Point<coord_t, 2>>>(
                         thrust::raw_pointer_cast(d_boxes.data()), n_inserted),
                     stream.cuda_stream());

        stream.Sync();
        sw.stop();
        total_insert_time += sw.ms();
      }

      std::cout << "Step " << i << " Geoms " << n_inserted << " Insert Time "
                << total_insert_time / config.repeat << " ms" << std::endl;
    }
  } else {
    double total_insert_time = 0;

    size_t n_batches = (boxes.size() + batch - 1) / batch;

    for (int repeat = 0; repeat < config.repeat; repeat++) {
      index.Clear();

      sw.start();
      for (int batch_id = 0; batch_id < n_batches; batch_id++) {
        size_t batch_begin = batch_id * batch;
        size_t batch_size =
            std::min(batch_begin + batch, boxes.size()) - batch_begin;

        index.Insert(rtspatial::ArrayView<
                         rtspatial::Envelope<rtspatial::Point<coord_t, 2>>>(
                         thrust::raw_pointer_cast(d_boxes.data()) + batch_begin,
                         batch_size),
                     stream.cuda_stream());
      }
      stream.Sync();
      sw.stop();
      total_insert_time += sw.ms();
    }
    total_insert_time /= config.repeat;

    std::cout << "Batch " << batch << " Geoms " << boxes.size()
              << " Insert Time " << total_insert_time << " ms Throughput "
              << boxes.size() / (total_insert_time / 1000) << " geoms/sec"
              << std::endl;
  }
  return ts;
}

time_stat RunDeletionRTSpatial(const std::vector<box_t> &boxes,
                               const BenchmarkConfig &config) {
  rtspatial::Stream stream;
  rtspatial::SpatialIndex<coord_t, 2> index;
  thrust::device_vector<rtspatial::Envelope<rtspatial::Point<coord_t, 2>>>
      d_boxes;
  thrust::device_vector<size_t> deleted_ids;
  rtspatial::Config idx_config;

  idx_config.ptx_root = std::string(RTSPATIAL_PTX_DIR);
  idx_config.max_geometries = boxes.size();
  CopyBoxes(boxes, d_boxes);

  index.Init(idx_config);
  time_stat ts;
  Stopwatch sw;

  ts.num_geoms = boxes.size();

  int batch = config.batch;

  if (batch == -1) {
    size_t n_steps = 100;
    size_t avg_gemos_per_step = (boxes.size() + n_steps - 1) / n_steps;
    size_t n_inserted = 0;

    for (size_t i = 0; i < n_steps; i++) {
      auto begin = i * avg_gemos_per_step;
      auto size = std::min(begin + avg_gemos_per_step, boxes.size()) - begin;
      double total_delete_time = 0;

      n_inserted += size;

      for (int repeat = 0; repeat < config.repeat; repeat++) {
        index.Clear();

        index.Insert(rtspatial::ArrayView<
                         rtspatial::Envelope<rtspatial::Point<coord_t, 2>>>(
                         thrust::raw_pointer_cast(d_boxes.data()), n_inserted),
                     stream.cuda_stream());
        stream.Sync();

        deleted_ids.resize(n_inserted);
        thrust::transform(thrust::cuda::par.on(stream.cuda_stream()),
                          thrust::make_counting_iterator<size_t>(0),
                          thrust::make_counting_iterator<size_t>(n_inserted),
                          deleted_ids.begin(), thrust::identity<size_t>());

        sw.start();
        index.Delete(rtspatial::ArrayView<size_t>(deleted_ids),
                     stream.cuda_stream());
        stream.Sync();
        sw.stop();
        total_delete_time += sw.ms();
      }

      std::cout << "Step " << i << " Geoms " << n_inserted << " Delete Time "
                << total_delete_time / config.repeat << " ms" << std::endl;
    }
  } else {
    double total_delete_time = 0;
    size_t n_batches = (boxes.size() + batch - 1) / batch;

    for (int repeat = 0; repeat < config.repeat; repeat++) {
      index.Clear();

      index.Insert(rtspatial::ArrayView<
                       rtspatial::Envelope<rtspatial::Point<coord_t, 2>>>(
                       thrust::raw_pointer_cast(d_boxes.data()), boxes.size()),
                   stream.cuda_stream());

      for (int batch_id = 0; batch_id < n_batches; batch_id++) {
        size_t batch_begin = batch_id * batch;
        size_t batch_end = std::min(batch_begin + batch, boxes.size());
        size_t batch_size = batch_end - batch_begin;

        deleted_ids.resize(batch_size);
        thrust::transform(thrust::cuda::par.on(stream.cuda_stream()),
                          thrust::make_counting_iterator<size_t>(batch_begin),
                          thrust::make_counting_iterator<size_t>(batch_end),
                          deleted_ids.begin(), thrust::identity<size_t>());
        sw.start();
        index.Delete(rtspatial::ArrayView<size_t>(deleted_ids),
                     stream.cuda_stream());
        stream.Sync();
        sw.stop();
        total_delete_time += sw.ms();
      }
    }

    total_delete_time /= config.repeat;

    std::cout << "Batch " << batch << " Geoms " << boxes.size()
              << " Delete Time " << total_delete_time << " ms Throughput "
              << boxes.size() / (total_delete_time / 1000) << " geoms/sec"
              << std::endl;
  }
  return ts;
}

