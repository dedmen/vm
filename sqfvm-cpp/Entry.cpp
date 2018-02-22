#include "virtualmachine.h"
#include "commandmap.h"
#include "value.h"
#include "vmstack.h"
#include <iostream>
#include <sstream>

#include "python_externc.h"


int main(int argc, char** argv)
{
	py_init(100000);
	wchar_t buff[2000];
	py_exec(L"diag_log 12", buff, 2000);
	py_uninit();

	auto vm = sqf::virtualmachine();
	//std::wstring line;
	//std::wcin >> line;
	int i = 0;
	wprintf(L"Please enter your SQF code.\nTo get the capabilities, use the `help__` instruction.\nTo run the code, Press <ENTER> twice.\n");
	std::wstring line;
	std::wstringstream sstream;
	do
	{
		wprintf(L"%d:\t", i++);
		std::getline(std::wcin, line);
		sstream << line << std::endl;
	} while (line.size() != 0);

	std::wcout << std::endl;

	/*
	sqf::commandmap::get().init();

	vm.out() << std::endl << L"ASSEMBLY" << std::endl;
	//vm.parse_assembly(L"push SCALAR 1; push SCALAR 1; callBinary +; push SCALAR 2; callBinary -;");
	//vm.parse_assembly(L"push SCALAR 1; push STRING 'test'; push STRING 'test''escape'; push SCALAR 1; push SCALAR 2; push SCALAR 3; callnular ntest; makeArray 4; makeArray 4;");
	//vm.parse_assembly(L"push SCALAR 12; assignToLocal _test;");
	vm.execute();
	
	vm.out() << std::endl << L"SQF" << std::endl;
	//vm.parse_sqf(L"1 + 1 - 2; 0; {1 + 2 * 3 - 1 * 4}; 6; utest 1; ntest; [1, 'test', 'test''escape', [1, 2, 3, ntest]]; private _test = 12; _test2 = 13; _test; globalvar;");
	//vm.parse_sqf(L"1 + 1 - 2;");
	//vm.parse_sqf(L"[1, 'test', 'test''escape', [1, 2, 3, ntest]]");
	//vm.parse_sqf(L"private _test = 12;");
	vm.execute();

	sqf::commandmap::get().uninit();
	*/

	sqf::commandmap::get().init();
	//auto cd = std::make_shared<sqf::configdata>();
	//vm.parse_config(L"class test { some = 1; stuff = \"some\"; foo = 2; bar = 3; }; class other : test { arr[] = {1, 2, 3, { \"1\", 2 } }; };", cd);
	vm.parse_sqf(sstream.str());
	vm.execute();
	std::shared_ptr<sqf::value> val;
	bool success;
	do {
		val = vm.stack()->popval(success);
		if (success)
		{
			vm.out() << L"[WORK]\t<" << sqf::type_str(val->dtype()) << L">\t" << val->as_string() << std::endl;
		}
	} while (success);
	sqf::commandmap::get().uninit();
	std::wcout << std::endl;
	system("pause");
}

/*
    0       4         0       4    0
[1, 2, 3] select ([0, 1, 2] select 2);

                  [0, 1, 2]        2
[1, 2, 3]        (          select  )
          select                     ;
                       [0, 1]
                 count          1
[1, 2, 3]                     +
          select                     ;

- (...) wie konstanten wert behandeln
- (...) rekursiv abarbeiten
- array mit index f�hren

*/