#include "BVH.h"
#include <algorithm>

BVHNode::BVHNode(const std::vector<shared_ptr<hittable>>& src_objects, size_t start, size_t end)
{
	// 复制一份
	auto objects = src_objects;

	int axis = random_int(0, 2);
	auto comparator = (axis == 0) ? box_x_compare :
						(axis == 1) ? box_y_compare :
						box_z_compare;
	
	size_t obj_span = end - start;

	if (obj_span == 1) 
	{
		left = right = objects[start];
	}
	else if (obj_span == 2) 
	{
		if (comparator(objects[start], objects[start + 1])) 
		{
			left = objects[start];
			right = objects[start + 1];
		}
		else 
		{
			left = objects[start + 1];
			right = objects[start];
		}
	}
	else 
	{
		std::sort(objects.begin() + start, objects.begin() + end, comparator);
		auto mid = start + obj_span / 2;
		left = make_shared<BVHNode>(objects, start, mid);
		right = make_shared<BVHNode>(objects, mid, end);
	}
	bbox = AABB(left->BoundingBox(), right->BoundingBox());

}

bool BVHNode::hit(const Ray& r, interval ray_t, hit_record& rec) const
{
	if (!bbox.hit(r, ray_t))
		return false;

	bool hitLeft = left->hit(r, ray_t, rec);
	bool hitRight = right->hit(r, 
		interval(ray_t.min, hitLeft ? rec.t : ray_t.max),
		rec);

	return hitLeft || hitRight;
}
