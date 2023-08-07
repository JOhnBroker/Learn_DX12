#ifndef IPASS_H
#define IPASS_H




class IPass
{
public:
	IPass() = default;
	virtual ~IPass() = 0;

	virtual void Render();
};


#endif // !IPASS_H