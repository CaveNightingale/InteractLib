# Interact Lib

This is a library for debugging interactive problems, you can run interactor completely inside your program so that you can debug your program easily.

It captures cin/cout and stdin/stdout and forward them to your interactor function, which is a coroutine.

## Features
- Launch easily, so don't need to setup pipes or anything.
- Share variables, so you can inspect variables in your interactor function.
- Log interactions, so you can see what happened between your program and interactor.
- Automatically count interactions, so you can see how many interactions happened.
- Deadlock detection, so you can see if your program forgets to output something.
- Error spreading, so the whole program will be terminated if an error occurs in either the program or the interactor.

## Usage
**This library only works on Linux x86_64 and Linux aarch64. The major difficulty porting to Windows is lacking `fopencookie()`/`funopen()` function to create custom `FILE*`.**
```cpp
#include <iostream>
#include <string>

int main() {
	int l = 0, r = 1000;
	while (l < r) {
		int m = (l + r + 1) / 2;
		std::cout << "? " << m << std::endl;
		std::string s;
		std::cin >> s;
		if (s == "<") {
			r = m - 1;
		} else {
			l = m;
		}
	}
	std::cout << "! " << l << std::endl;
}

#ifndef ONLINE_JUDGE
#include "interactlib.hh"

static cave::Interactor interactor([](auto &test) {
	test.set_max_count(15); // Set Maximum Interaction Count. It will abort if the interaction count exceeds this value.
	int val;
	test.sys_cin >> val; // Read the number to guess from terminal
	while (true) {
		std::string op;
		test >> op; // Read from the program
		if (op == "?") {
			int x;
			test >> x; // Read the guess from the program
			if (val < x) {
				test << "<" << std::endl; // Send the response to the program
			} else {
				test << ">=" << std::endl; // Send the response to the program
			}
		} else {
			int x;
			test >> x; // Read the guess from the program
			if (x == val) {
				test.sys_cout << "Correct" << std::endl; // Send the response to the terminal
				break;
			} else {
				test.sys_cout << "Incorrect" << std::endl; // Send the response to the terminal
				abort();
			}
		}
	}
});

#endif
```