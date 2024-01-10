#ifndef HITTABLE_LIST_H
#define HITTABLE_LIST_H

#include "hittable.h"
#include <vector>

class hittable_list :public hittable 
{
public:
	hittable_list() {}
	hittable_list(shared_ptr<hittable> object) {}

	void clear() { objects.clear(); objects.shrink_to_fit(); }
	void add(shared_ptr<hittable> obj) { objects.push_back(obj); }

	virtual bool hit(const ray& r, double t_min, double t_max, hit_record& rec)const override;

public: 
	std::vector<shared_ptr<hittable>> objects;
};

bool hittable_list::hit(const ray& r, double t_min, double t_max, hit_record& rec)const 
{
	hit_record tem_rec;
	bool hit_anything = false;
	double closest_far = t_max;

	for (const auto& obj : objects) 
	{
		// closest_far 作为 t_max进行射线检测，所以能保证rec存的是离相机距离最近
		if (obj->hit(r, t_min, closest_far, tem_rec)) 
		{
			hit_anything = true;
			closest_far = tem_rec.t;
			rec = tem_rec;
		}
	}
	return hit_anything;
}


#endif // !HITTABLE_LIST_H
