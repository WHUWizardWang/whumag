#pragma once

#include <vector>
#include <array>
#include <algorithm>
#include <limits>
#include <queue>
#include <utility>
#include <cmath>
#include <numeric>
#include <set>
#include <iostream>
#include <mutex>
#include <atomic>
#include "nanoflann.hpp"
using nanoflann::KDTreeSingleIndexAdaptor;
using nanoflann::L2_Simple_Adaptor;

struct PointCloud {
    struct P { double x,y; };
    std::vector<P> pts;
    // 接口：返回点数
    inline size_t kdtree_get_point_count() const { return pts.size(); }
    // 接口：第 idx 个点的第 dim 维坐标
    inline double kdtree_get_pt(size_t idx, size_t dim) const {
        return dim==0 ? pts[idx].x : pts[idx].y;
    }
    // 可选：bounding-box，返回 false 让 KD-Tree 使用默认值
    template <class BBOX> bool kdtree_get_bbox(BBOX&) const { return false; }
};
// KD-Tree 类型，用 2 维 L2 距离
using KDTree2D = KDTreeSingleIndexAdaptor<
    L2_Simple_Adaptor<double, PointCloud>,
    PointCloud,
    2, /* dim */
    size_t /* index type, size_t for simplicity */
    >;

// 添加凸包算法
// Graham扫描算法计算2D点集的凸包
class ConvexHull {
public:
    // 用于Graham扫描的极角排序比较器
    struct PolarAngleComparator {
        std::vector<double>& x;
        std::vector<double>& y;
        size_t pivot_idx;

        PolarAngleComparator(std::vector<double>& x_vals, std::vector<double>& y_vals, size_t pivot)
            : x(x_vals), y(y_vals), pivot_idx(pivot) {}

        bool operator()(size_t a, size_t b) const {
            double cross_product = (x[a] - x[pivot_idx]) * (y[b] - y[pivot_idx]) -
                                   (y[a] - y[pivot_idx]) * (x[b] - x[pivot_idx]);

            if (std::abs(cross_product) < 1e-10) {
                // 共线的情况下，选择距离更远的点
                double dist_a = std::pow(x[a] - x[pivot_idx], 2) + std::pow(y[a] - y[pivot_idx], 2);
                double dist_b = std::pow(x[b] - x[pivot_idx], 2) + std::pow(y[b] - y[pivot_idx], 2);
                return dist_a < dist_b;
            }

            return cross_product > 0;
        }
    };

    // 计算叉积，用于判断转向方向
    static double cross(double x1, double y1, double x2, double y2, double x3, double y3) {
        return (x2 - x1) * (y3 - y1) - (y2 - y1) * (x3 - x1);
    }

    // Graham扫描算法计算凸包
    static std::vector<size_t> compute(std::vector<double>& x, std::vector<double>& y) {
        size_t n = x.size();
        if (n < 3) {
            // 少于3个点，所有点都在凸包上
            std::vector<size_t> result(n);
            std::iota(result.begin(), result.end(), 0);
            return result;
        }

        // 找到y坐标最小的点（如果有多个，取x最小的）作为pivot
        size_t pivot = 0;
        for (size_t i = 1; i < n; i++) {
            if (y[i] < y[pivot] || (y[i] == y[pivot] && x[i] < x[pivot])) {
                pivot = i;
            }
        }

        // 将pivot与第一个点交换
        std::swap(x[0], x[pivot]);
        std::swap(y[0], y[pivot]);
        pivot = 0;

        // 创建索引数组并按极角排序
        std::vector<size_t> indices(n);
        std::iota(indices.begin(), indices.end(), 0);

        // 排除pivot本身
        std::vector<size_t> sorted_indices(indices.begin() + 1, indices.end());
        PolarAngleComparator comp(x, y, pivot);
        std::sort(sorted_indices.begin(), sorted_indices.end(), comp);

        // 重建索引，将pivot放回第一位
        indices.clear();
        indices.push_back(pivot);
        indices.insert(indices.end(), sorted_indices.begin(), sorted_indices.end());

        // Graham扫描算法
        std::vector<size_t> hull;
        hull.push_back(indices[0]);
        hull.push_back(indices[1]);

        for (size_t i = 2; i < n; i++) {
            while (hull.size() > 1 &&
                   cross(x[hull[hull.size() - 2]], y[hull[hull.size() - 2]],
                         x[hull[hull.size() - 1]], y[hull[hull.size() - 1]],
                         x[indices[i]], y[indices[i]]) <= 0) {
                hull.pop_back();
            }
            hull.push_back(indices[i]);
        }

        return hull;
    }

    // 检查点(px,py)是否在由indices指定的凸多边形内
    static bool pointInConvexHull(double px, double py,
                                  const std::vector<double>& x,
                                  const std::vector<double>& y,
                                  const std::vector<size_t>& hull) {
        size_t n = hull.size();
        if (n < 3) return false;

        // 检查点是否在每条边的"内侧"
        for (size_t i = 0; i < n; i++) {
            size_t j = (i + 1) % n;

            double xi = x[hull[i]];
            double yi = y[hull[i]];
            double xj = x[hull[j]];
            double yj = y[hull[j]];

            if (cross(xi, yi, xj, yj, px, py) < 0) {
                return false;
            }
        }

        return true;
    }

    // 计算点到线段的最短距离
    static double pointToSegmentDistance(double px, double py,
                                         double x1, double y1,
                                         double x2, double y2) {
        double A = px - x1;
        double B = py - y1;
        double C = x2 - x1;
        double D = y2 - y1;

        double dot = A * C + B * D;
        double len_sq = C * C + D * D;
        double param = -1.0;

        if (len_sq != 0) {
            param = dot / len_sq;
        }

        double xx, yy;

        if (param < 0) {
            xx = x1;
            yy = y1;
        } else if (param > 1) {
            xx = x2;
            yy = y2;
        } else {
            xx = x1 + param * C;
            yy = y1 + param * D;
        }

        double dx = px - xx;
        double dy = py - yy;

        return std::sqrt(dx * dx + dy * dy);
    }

    // 找到点到凸包的最短距离和最近点
    static std::pair<double, size_t> nearestPointOnConvexHull(
        double px, double py,
        const std::vector<double>& x,
        const std::vector<double>& y,
        const std::vector<size_t>& hull) {

        double min_dist = std::numeric_limits<double>::max();
        size_t nearest_idx = 0;

        size_t n = hull.size();
        for (size_t i = 0; i < n; i++) {
            size_t j = (i + 1) % n;

            double dist = pointToSegmentDistance(px, py, x[hull[i]], y[hull[i]], x[hull[j]], y[hull[j]]);

            if (dist < min_dist) {
                min_dist = dist;

                // 计算最近点是线段上的哪个点
                double x1 = x[hull[i]];
                double y1 = y[hull[i]];
                double x2 = x[hull[j]];
                double y2 = y[hull[j]];

                double A = px - x1;
                double B = py - y1;
                double C = x2 - x1;
                double D = y2 - y1;

                double dot = A * C + B * D;
                double len_sq = C * C + D * D;
                double param = dot / len_sq;

                if (param <= 0) {
                    nearest_idx = hull[i];
                } else if (param >= 1) {
                    nearest_idx = hull[j];
                } else {
                    // 最近点在线段上，选择线段上两点中的一个
                    double dist_i = std::pow(x[hull[i]] - px, 2) + std::pow(y[hull[i]] - py, 2);
                    double dist_j = std::pow(x[hull[j]] - px, 2) + std::pow(y[hull[j]] - py, 2);
                    nearest_idx = (dist_i < dist_j) ? hull[i] : hull[j];
                }
            }
        }

        return {min_dist, nearest_idx};
    }
};

// KDTree实现
template<typename T, size_t Dim>
class KDTree {
private:
    struct Node {
        std::array<T, Dim> point;
        size_t idx;
        Node* left = nullptr;
        Node* right = nullptr;

        Node(const std::array<T, Dim>& p, size_t index) : point(p), idx(index) {}
        ~Node() {
            delete left;
            delete right;
        }
    };

    Node* root = nullptr;

    Node* buildTree(std::vector<std::pair<std::array<T, Dim>, size_t>>& points,
                    int start, int end, int depth) {
        if (start >= end) return nullptr;

        // 根据深度选择坐标轴
        int axis = depth % Dim;

        // 沿选定轴对点进行排序
        int mid = start + (end - start) / 2;
        std::nth_element(
            points.begin() + start,
            points.begin() + mid,
            points.begin() + end,
            [axis](const auto& a, const auto& b) {
                return a.first[axis] < b.first[axis];
            }
            );

        // 创建节点并构建子树
        Node* node = new Node(points[mid].first, points[mid].second);
        node->left = buildTree(points, start, mid, depth + 1);
        node->right = buildTree(points, mid + 1, end, depth + 1);
        return node;
    }

    void nearestSearch(Node* node, const std::array<T, Dim>& query,
                       std::priority_queue<std::pair<T, size_t>>& pq, int k, int depth) const {
        if (!node) return;

        // 计算距离
        T dist = 0;
        for (size_t i = 0; i < Dim; ++i) {
            dist += std::pow(query[i] - node->point[i], 2);
        }

        // 添加到优先队列
        if (pq.size() < k || dist < pq.top().first) {
            if (pq.size() == k) pq.pop();
            pq.push({dist, node->idx});
        }

        // 确定优先搜索哪个子树
        int axis = depth % Dim;
        T diff = query[axis] - node->point[axis];

        Node* firstSearch = (diff < 0) ? node->left : node->right;
        Node* secondSearch = (diff < 0) ? node->right : node->left;

        // 搜索较近的子树
        nearestSearch(firstSearch, query, pq, k, depth + 1);

        // 如果另一个子树可能包含更近的点，也搜索它。
        // pq.size() < k 必须放在前面短路求值：pq为空时 pq.top() 是未定义行为。
        if (pq.size() < k || std::pow(diff, 2) < pq.top().first) {
            nearestSearch(secondSearch, query, pq, k, depth + 1);
        }
    }

public:
    KDTree(const std::vector<std::array<T, Dim>>& points) {
        std::vector<std::pair<std::array<T, Dim>, size_t>> indexed_points;
        indexed_points.reserve(points.size());
        for (size_t i = 0; i < points.size(); ++i) {
            indexed_points.push_back({points[i], i});
        }
        root = buildTree(indexed_points, 0, indexed_points.size(), 0);
    }

    ~KDTree() {
        delete root;
    }

    // 查找最多k个最近邻（树中点数少于k时，返回全部点，不再用哨兵值填充）
    std::vector<std::pair<T, size_t>> nearest(const std::array<T, Dim>& query, int k) const {
        std::priority_queue<std::pair<T, size_t>> pq;

        nearestSearch(root, query, pq, k, 0);

        std::vector<std::pair<T, size_t>> result;
        while (!pq.empty()) {
            result.push_back(pq.top());
            pq.pop();
        }
        std::reverse(result.begin(), result.end());
        return result;
    }
};
