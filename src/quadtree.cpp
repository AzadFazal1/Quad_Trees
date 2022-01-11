
#include "quadtree.h"

namespace cs225 {

quadtree::quadtree() : root_{nullptr} {}

quadtree::quadtree(const epng::png &source, uint64_t resolution) {
  resolution_ = resolution;
  new_tree(root_, source, resolution_, 0, 0);
}

void quadtree::new_tree(std::unique_ptr<node> &subroot, const epng::png &source,
                        uint64_t resolution, uint64_t x, uint64_t y) {

  if (resolution == 1) {

    subroot = std::make_unique<node>();
    subroot->element = *(source(x, y));

    subroot->northwest = nullptr;
    subroot->northeast = nullptr;
    subroot->southwest = nullptr;
    subroot->southeast = nullptr;
  }

  else {
    subroot = std::make_unique<node>();

    new_tree(subroot->northwest, source, resolution / 2, x, y);
    new_tree(subroot->northeast, source, resolution / 2, x + (resolution / 2),
             y);
    new_tree(subroot->southwest, source, resolution / 2, x,
             y + (resolution / 2));
    new_tree(subroot->southeast, source, resolution / 2, x + (resolution / 2),
             y + (resolution / 2));

    subroot->element.red =
        (subroot->northwest->element.red + subroot->northeast->element.red +
         subroot->southwest->element.red + subroot->southeast->element.red) /
        4;
    subroot->element.green =
        (subroot->northwest->element.green + subroot->northeast->element.green +
         subroot->southwest->element.green +
         subroot->southeast->element.green) /
        4;
    subroot->element.blue =
        (subroot->northwest->element.blue + subroot->northeast->element.blue +
         subroot->southwest->element.blue + subroot->southeast->element.blue) /
        4;
    subroot->element.alpha =
        (subroot->northwest->element.alpha + subroot->northeast->element.alpha +
         subroot->southwest->element.alpha +
         subroot->southeast->element.alpha) /
        4;
  }
}

void quadtree::clear(std::unique_ptr<node> &subroot) {
  if (!subroot) {
    return;
  } else {
    clear(subroot->northwest);
    clear(subroot->northeast);
    clear(subroot->southwest);
    clear(subroot->southeast);
    subroot = nullptr;
  }
}

quadtree::quadtree(const quadtree &other) {
  resolution_ = other.resolution_;

  root_ = copy(other.root_);
}

auto quadtree::copy(const std::unique_ptr<node> &subroot)
    -> std::unique_ptr<node> {

  if (!subroot) {
    return {};
  }

  auto newnode = std::make_unique<node>();
  newnode->element = subroot->element;

  // std::cout<<subroot->element<<std::endl;

  newnode->northwest = copy(subroot->northwest);
  newnode->northeast = copy(subroot->northeast);
  newnode->southwest = copy(subroot->southwest);
  newnode->southeast = copy(subroot->southeast);
  return newnode;
}

quadtree::quadtree(quadtree &&other) {
  if (this == &other) {
    return;
  }
  swap(other);
}

quadtree &quadtree::operator=(quadtree other) {
  if (this == &other) {
    return *this;
  }

  quadtree temp;
  swap(temp);
  resolution_ = other.resolution_;
  root_ = copy(other.root_);

  return *this;
}

void quadtree::swap(quadtree &other) {
  std::swap(resolution_, other.resolution_);
  std::swap(root_, other.root_);
}

void quadtree::build_tree(const epng::png &source, uint64_t resolution) {

  quadtree temp(source, resolution);
  swap(temp);
  new_tree(root_, source, resolution, 0, 0);
}

const epng::rgba_pixel &quadtree::operator()(uint64_t x, uint64_t y) const {

  if (x > resolution_ || y > resolution_ || !root_) {
    throw std::out_of_range("Coordinates out of range");
  }
  return get_pixel(x, y, root_.get(), resolution_, 0, 0);
}
epng::rgba_pixel &quadtree::get_pixel(uint64_t x, uint64_t y,
                                      node *const subroot, uint64_t resolution,
                                      uint64_t a, uint64_t b) const {

  if (!subroot->northwest || resolution == 1) {

    return subroot->element;
  } else {

    if ((x < a + resolution / 2) && (x >= a) && (y < b + resolution / 2) &&
        (y >= b)) {

      return get_pixel(x, y, subroot->northwest.get(), resolution / 2, a, b);

    } else if ((x >= a + resolution / 2) && (x < a + resolution) &&
               (y < b + resolution / 2) && (y >= b)) {

      return get_pixel(x, y, subroot->northeast.get(), resolution / 2,
                       a + resolution / 2, b);

    } else if ((x < a + resolution / 2) && (x >= a) &&
               (y >= b + resolution / 2) && (y <= b + resolution)) {

      return get_pixel(x, y, subroot->southwest.get(), resolution / 2, a,
                       b + resolution / 2);
    } else {

      return get_pixel(x, y, subroot->southeast.get(), resolution / 2,
                       a + resolution / 2, b + resolution / 2);
    }
  }
}

epng::png quadtree::decompress() const {

  epng::png object;

  object.resize(resolution_, resolution_);

  for (uint64_t i = 0; i < resolution_; i++) {

    for (uint64_t j = 0; j < resolution_; j++) {
      *(object(i, j)) = get_pixel(i, j, root_.get(), resolution_, 0, 0);
    }
  }

  return object;
}

void quadtree::rotate_clockwise() {
  if (!root_) {
    return;
  }
  rotate(root_);
}
void quadtree::rotate(std::unique_ptr<node> &subroot) {
  if (subroot->northwest) {
    auto temp = std::make_unique<node>();
    temp = std::move(subroot->northwest);
    subroot->northwest = std::move(subroot->southwest);
    subroot->southwest = std::move(subroot->southeast);
    subroot->southeast = std::move(subroot->northeast);
    subroot->northeast = std::move(temp);

    rotate(subroot->northwest);
    rotate(subroot->northeast);
    rotate(subroot->southwest);
    rotate(subroot->southeast);
  }
}
void quadtree::prune(uint32_t tolerance) {
  if (!root_) {
    return;
  }
  prune(root_, tolerance);
}

void quadtree::prune(std::unique_ptr<node> &subroot, uint32_t tolerance) {
  if (subroot) {
    if (prunable(subroot->northeast.get(), subroot.get(), tolerance) &&
        prunable(subroot->northwest.get(), subroot.get(), tolerance) &&
        prunable(subroot->southeast.get(), subroot.get(), tolerance) &&
        prunable(subroot->southwest.get(), subroot.get(), tolerance)) {

      clear(subroot->northeast);
      clear(subroot->northwest);
      clear(subroot->southeast);
      clear(subroot->southwest);
    }
    prune(subroot->northeast, tolerance);
    prune(subroot->northwest, tolerance);
    prune(subroot->southeast, tolerance);
    prune(subroot->southwest, tolerance);
  }
}
bool quadtree::prunable(node *subroot, node *parent, uint32_t tolerance) const {
  if (!subroot) {
    return false;
  }
  uint32_t x;
  if (!subroot->northeast) {
    x = ((subroot->element.red - parent->element.red) *
         (subroot->element.red - parent->element.red)) +
        ((subroot->element.green - parent->element.green) *
         (subroot->element.green - parent->element.green)) +
        ((subroot->element.blue - parent->element.blue) *
         (subroot->element.blue - parent->element.blue)) +
        ((subroot->element.alpha - parent->element.alpha) *
         (subroot->element.alpha - parent->element.alpha));
    if (x <= tolerance)
      return true;
    else
      return false;
  } else {
    return prunable(subroot->northeast.get(), parent, tolerance) &&
           prunable(subroot->northwest.get(), parent, tolerance) &&
           prunable(subroot->southeast.get(), parent, tolerance) &&
           prunable(subroot->southwest.get(), parent, tolerance);
  }
}

uint64_t quadtree::pruned_size(uint32_t tolerance) const {
  if (!root_)
    return 0;
  return (resolution_ * resolution_) - pruned_size(root_.get(), tolerance);
}

uint64_t quadtree::pruned_size(node *subroot, uint32_t tolerance) const {
  if (subroot) {
    if (prunable(subroot->northeast.get(), subroot, tolerance) &&
        prunable(subroot->northwest.get(), subroot, tolerance) &&
        prunable(subroot->southeast.get(), subroot, tolerance) &&
        prunable(subroot->southwest.get(), subroot, tolerance)) {

      return leaf_count(subroot->northeast.get()) +
             leaf_count(subroot->northwest.get()) +
             leaf_count(subroot->southeast.get()) +
             leaf_count(subroot->southwest.get()) - 1;
    }

    return pruned_size(subroot->northeast.get(), tolerance) +
           pruned_size(subroot->northwest.get(), tolerance) +
           pruned_size(subroot->southeast.get(), tolerance) +
           pruned_size(subroot->southwest.get(), tolerance);
  } else
    return 0;
}
uint64_t quadtree::leaf_count(node *subroot) const {
  if (!subroot) {
    return 0;
  } else if (!subroot->northeast) {
    return 1;
  } else
    return leaf_count(subroot->northeast.get()) +
           leaf_count(subroot->northwest.get()) +
           leaf_count(subroot->southeast.get()) +
           leaf_count(subroot->southwest.get());
}
uint32_t quadtree::ideal_prune(uint64_t num_leaves) const {} // namespace cs225

} // namespace cs225
