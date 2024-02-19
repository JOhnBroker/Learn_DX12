#ifndef BVH_H
#define BVH_H

#include "Common/common.h"

#include "hittable_list.h"

class BVHNode :public hittable 
{
public:
	BVHNode(const hittable_list& list)
		: BVHNode(list.objects, 0, list.objects.size()){}
	BVHNode(const std::vector<shared_ptr<hittable>>& src_objects, size_t start, size_t end);

	bool hit(const Ray& r, interval ray_t, hit_record& rec)const;
	AABB BoundingBox()const override { return bbox; }

private:
	static bool box_compare(
		const shared_ptr<hittable> a, const shared_ptr<hittable> b, int axis_index) 
	{
		return a->BoundingBox().axis(axis_index).min < b->BoundingBox().axis(axis_index).min;
	}
	static bool box_x_compare(
		const shared_ptr<hittable> a, const shared_ptr<hittable> b) 
	{
		return box_compare(a, b, 0);
	}
	static bool box_y_compare(
		const shared_ptr<hittable> a, const shared_ptr<hittable> b)
	{
		return box_compare(a, b, 1);
	}
	static bool box_z_compare(
		const shared_ptr<hittable> a, const shared_ptr<hittable> b)
	{
		return box_compare(a, b, 2);
	}

private:
	shared_ptr<hittable> left;
	shared_ptr<hittable> right;
	AABB bbox;
};


#endif // !BVH_H


