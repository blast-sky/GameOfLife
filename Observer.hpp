#pragma once
#include <vector>

template<class tEvent>
class Observer
{
public:
	virtual void newEvent(tEvent event) = 0;
};

template<class tEvent>
class Subject
{
private:
	std::vector<Observer<tEvent>*> observers;
public:
	void sendEvent(tEvent event);
	void addObserver(Observer<tEvent>& observer);
	void deleteObserver(Observer<tEvent>& observer);
};



template<class tEvent>
inline void Subject<tEvent>::sendEvent(tEvent event)
{
	for (Observer<tEvent>* observer : observers)
		observer->newEvent(event);
}

template<class tEvent>
inline void Subject<tEvent>::addObserver(Observer<tEvent>& observer)
{
	observers.push_back(&observer);
}

template<class tEvent>
inline void Subject<tEvent>::deleteObserver(Observer<tEvent>& observer)
{
	auto it = std::find(observers.begin(), observers.end(), &observer);
	if (it != observers.end()) observers.erase(it);
}
