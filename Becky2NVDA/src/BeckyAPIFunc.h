#include <string>
#include <vector>

#include <Windows.h>


template <typename FUNC_TYPE>
class BeckyAPIFunc
{
public:
	BeckyAPIFunc(std::string funcName);
	FUNC_TYPE getFuncPointer() const;

private:

	const std::string m_funcName;
	bool m_funcRetrieved = false;
	FUNC_TYPE m_funcPointer = nullptr;
	static std::vector<BeckyAPIFunc<FUNC_TYPE> *> instances;

	void retrieveAPIFunc();
	
};

template <typename FUNC_TYPE>
BeckyAPIFunc<FUNC_TYPE>::BeckyAPIFunc(std::string funcName) : m_funcName(funcName) {
	instances.push_back(this);
}

template <typename FUNC_TYPE>
void BeckyAPIFunc<FUNC_TYPE>::retrieveAPIFunc() {
	FARPROC res = GetProcAddress(GetModuleHandle(nullptr), m_funcName.c_str());
	m_funcRetrieved = (res != nullptr);
	if (m_funcRetrieved) {
		static_assert(sizeof(FUNC_TYPE) == sizeof(res));
		FUNC_TYPE funcT {};
		memcpy(&funcT, &res, sizeof(FUNC_TYPE));
		m_funcPointer = funcT;
	}
}


template <typename FUNC_TYPE>
FUNC_TYPE BeckyAPIFunc<FUNC_TYPE>::getFuncPointer() const{
	return m_funcPointer;
}
