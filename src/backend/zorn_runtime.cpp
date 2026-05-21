#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>
extern "C" {
struct ZornTensor {
  int64_t ndim;
  int64_t *sizes;
  double *data;
};
ZornTensor *zorn_tensor_create(int64_t ndim, int64_t *sizes) {
  ZornTensor *t = (ZornTensor *)malloc(sizeof(ZornTensor));
  t->ndim = ndim;
  t->sizes = (int64_t *)malloc(sizeof(int64_t) * ndim);
  int64_t total_size = 1;
  for (int64_t i = 0; i < ndim; i++) {
    t->sizes[i] = sizes[i];
    total_size *= sizes[i];
  }
  if (total_size == 0)
    total_size = 1;
  t->data = (double *)malloc(sizeof(double) * total_size);
  for (int64_t i = 0; i < total_size; i++)
    t->data[i] = 0.0;
  return t;
}
void zorn_tensor_fill(ZornTensor *t, double val) {
  int64_t total_size = 1;
  for (int64_t i = 0; i < t->ndim; i++) {
    total_size *= t->sizes[i];
  }
  if (total_size == 0) total_size = 1;
  for (int64_t i = 0; i < total_size; i++) {
    t->data[i] = val;
  }
}
ZornTensor *zorn_mat_mul(ZornTensor *a, ZornTensor *b) {
  if (a->ndim != 2 || b->ndim != 2)
    return nullptr;
  int64_t m = a->sizes[0];
  int64_t k = a->sizes[1];
  int64_t n = b->sizes[1];
  int64_t new_sizes[2] = {m, n};
  ZornTensor *c = zorn_tensor_create(2, new_sizes);
  for (int64_t i = 0; i < m; i++) {
    for (int64_t j = 0; j < n; j++) {
      double sum = 0;
      for (int64_t l = 0; l < k; l++) {
        sum += a->data[i * k + l] * b->data[l * n + j];
      }
      c->data[i * n + j] = sum;
    }
  }
  return c;
}
ZornTensor *zorn_transpose(ZornTensor *a) {
  if (a->ndim != 2)
    return nullptr;
  int64_t m = a->sizes[0];
  int64_t n = a->sizes[1];
  int64_t new_sizes[2] = {n, m};
  ZornTensor *c = zorn_tensor_create(2, new_sizes);
  for (int64_t i = 0; i < m; i++) {
    for (int64_t j = 0; j < n; j++) {
      c->data[j * m + i] = a->data[i * n + j];
    }
  }
  return c;
}
ZornTensor *zorn_tensor_add(ZornTensor *a, ZornTensor *b) {
  ZornTensor *c = zorn_tensor_create(a->ndim, a->sizes);
  int64_t total = 1;
  for (int64_t i = 0; i < a->ndim; i++)
    total *= a->sizes[i];
  for (int64_t i = 0; i < total; i++)
    c->data[i] = a->data[i] + b->data[i];
  return c;
}
ZornTensor *zorn_tensor_sub(ZornTensor *a, ZornTensor *b) {
  ZornTensor *c = zorn_tensor_create(a->ndim, a->sizes);
  int64_t total = 1;
  for (int64_t i = 0; i < a->ndim; i++)
    total *= a->sizes[i];
  for (int64_t i = 0; i < total; i++)
    c->data[i] = a->data[i] - b->data[i];
  return c;
}
ZornTensor *zorn_tensor_mul(ZornTensor *a, ZornTensor *b) {
  ZornTensor *c = zorn_tensor_create(a->ndim, a->sizes);
  int64_t total = 1;
  for (int64_t i = 0; i < a->ndim; i++)
    total *= a->sizes[i];
  for (int64_t i = 0; i < total; i++)
    c->data[i] = a->data[i] * b->data[i];
  return c;
}
ZornTensor *zorn_tensor_div(ZornTensor *a, ZornTensor *b) {
  ZornTensor *c = zorn_tensor_create(a->ndim, a->sizes);
  int64_t total = 1;
  for (int64_t i = 0; i < a->ndim; i++)
    total *= a->sizes[i];
  for (int64_t i = 0; i < total; i++)
    c->data[i] = a->data[i] / b->data[i];
  return c;
}
ZornTensor *zorn_tensor_mod(ZornTensor *a, ZornTensor *b) {
  ZornTensor *c = zorn_tensor_create(a->ndim, a->sizes);
  int64_t total = 1;
  for (int64_t i = 0; i < a->ndim; i++)
    total *= a->sizes[i];
  for (int64_t i = 0; i < total; i++)
    c->data[i] = std::fmod(a->data[i], b->data[i]);
  return c;
}
ZornTensor *zorn_inverse(ZornTensor *a) {
  if (a->ndim != 2 || a->sizes[0] != a->sizes[1])
    return nullptr;
  int64_t n = a->sizes[0];
  ZornTensor *c = zorn_tensor_create(2, a->sizes);
  for (int64_t i = 0; i < n; i++)
    c->data[i * n + i] = 1.0;
  std::vector<double> aug(n * n);
  for (int64_t i = 0; i < n * n; i++)
    aug[i] = a->data[i];
  for (int64_t col = 0; col < n; col++) {
    int64_t pivot = -1;
    double max_val = 0.0;
    for (int64_t row = col; row < n; row++) {
      double val = std::abs(aug[row * n + col]);
      if (val > max_val) {
        max_val = val;
        pivot = row;
      }
    }
    if (pivot == -1 || max_val < 1e-12) {
      for (int64_t i = 0; i < n * n; i++)
        c->data[i] = 0.0;
      return c;
    }
    if (pivot != col) {
      for (int64_t j = 0; j < n; j++) {
        std::swap(aug[col * n + j], aug[pivot * n + j]);
        std::swap(c->data[col * n + j], c->data[pivot * n + j]);
      }
    }
    double scale = aug[col * n + col];
    for (int64_t j = 0; j < n; j++) {
      aug[col * n + j] /= scale;
      c->data[col * n + j] /= scale;
    }
    for (int64_t row = 0; row < n; row++) {
      if (row == col) continue;
      double factor = aug[row * n + col];
      for (int64_t j = 0; j < n; j++) {
        aug[row * n + j] -= factor * aug[col * n + j];
        c->data[row * n + j] -= factor * c->data[col * n + j];
      }
    }
  }
  return c;
}
ZornTensor *zorn_tensor_shape(ZornTensor *a) {
  int64_t sizes[1] = {a->ndim};
  ZornTensor *c = zorn_tensor_create(1, sizes);
  for (int64_t i = 0; i < a->ndim; i++)
    c->data[i] = (double)a->sizes[i];
  return c;
}
double zorn_tensor_get(ZornTensor *a, int64_t num_indices, int64_t *indices) {
  if (num_indices != a->ndim)
    return 0.0;
  int64_t flat_idx = 0;
  int64_t stride = 1;
  for (int64_t i = a->ndim - 1; i >= 0; i--) {
    flat_idx += indices[i] * stride;
    stride *= a->sizes[i];
  }
  return a->data[flat_idx];
}
void zorn_tensor_set(ZornTensor *a, int64_t num_indices, int64_t *indices,
                     double val) {
  if (num_indices != a->ndim)
    return;
  int64_t flat_idx = 0;
  int64_t stride = 1;
  for (int64_t i = a->ndim - 1; i >= 0; i--) {
    flat_idx += indices[i] * stride;
    stride *= a->sizes[i];
  }
  a->data[flat_idx] = val;
}
void zorn_print_tensor(ZornTensor *a) {
  std::cout << "[Tensor dims=" << a->ndim << " shapes=(";
  for (int i = 0; i < a->ndim; i++) {
    std::cout << a->sizes[i] << (i == a->ndim - 1 ? "" : ",");
  }
  std::cout << ")]\n";
}
void zorn_print_int(int64_t a) { std::cout << a << std::endl; }
void zorn_print_float(double a) { std::cout << a << std::endl; }
void zorn_print_bool(bool a) {
  std::cout << (a ? "true" : "false") << std::endl;
}
void zorn_print_tensor_nn(ZornTensor *a) {
  std::cout << "[Tensor dims=" << a->ndim << " shapes=(";
  for (int i = 0; i < a->ndim; i++) {
    std::cout << a->sizes[i] << (i == a->ndim - 1 ? "" : ",");
  }
  std::cout << ")]";
}
void zorn_print_int_nn(int64_t a) { std::cout << a; }
void zorn_print_float_nn(double a) { std::cout << a; }
void zorn_print_bool_nn(bool a) { std::cout << (a ? "true" : "false"); }
void zorn_print_string(const char *a) { std::cout << a << std::endl; }
void zorn_print_string_nn(const char *a) { std::cout << a; }
}
