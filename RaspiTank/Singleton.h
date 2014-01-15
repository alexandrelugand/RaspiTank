#pragma once 
#include <mutex>

using namespace std;

//////////////////////////////////////////////////////
// Classe singleton générique (Thread safe)
//////////////////////////////////////////////////////

template <typename T>
class Singleton
{
private:
	static T* instance;	//Instance de la classe T
	static mutex mtx;	//Thread safe (en multithread)

protected:
	Singleton() { }
	virtual ~Singleton() { }

public:
	static T& GetInstance()
	{
		mtx.lock();
		if (instance == NULL)
		{
			instance = new T;
		}
		mtx.unlock();
		return *instance;	//Retourne l'instance unique
	};

	static void Kill()
	{
		mtx.lock();
		if (instance != NULL)
		{
			delete instance; 	//Libère l'instance unique	
			instance = NULL;
		}
		mtx.unlock();
	};
};

//Attributs statiques du template CSingleton
template <typename T>
T* Singleton<T>::instance = NULL; //Instance du singleton  
template <typename T>
mutex Singleton<T>::mtx;		 //Mutex pour assurer que le singleton est ThreadSafe

