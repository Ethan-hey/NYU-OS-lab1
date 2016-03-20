#include <cstring>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <string.h>
#include <stdlib.h>
template <typename T>
    std::string to_string(T value)
    {
      //create an output string stream
      std::ostringstream os ;

      //throw the value into the string stream
      os << value ;

      //convert the string stream into a string and return
      return os.str() ;
    }
using namespace std;
// symbol table
map<string, string> m;
// rule 9. offset of each module
map<int, int> moduleSize;
//rule 7, used in pass two
map<string, bool> usedOrNot;

int total;
int offset;
int indexAddr;
int moduleIndex = 1;
int lastline = 0;
int line = 1;
int lineoffset = 0;
ifstream fin;

int ReturnNum(string s) {
	for (int i = 0; i < s.length(); i++) {
		if (s.at(i) == '*' || s.at(i) == '/')
			return atoi(s.substr(0, i).c_str());
	}
	return atoi(s.c_str());
}
void ParseError(int errcode, int len = 0) {
	static char* errstr[] = { "NUM_EXPECTED", // Number expect
			"SYM_EXPECTED", // Symbol Expected
			"ADDR_EXPECTED", // Addressing Expected
			"SYM_TOLONG", // Symbol Name is to long
			"TO_MANY_DEF_IN_MODULE", // > 16
			"TO_MANY_USE_IN_MODULE", //>16
			"TO_MANY_INSTR" }; // total num_instr exceeds memory size (512)
	int first = lineoffset == 0 ? line - 1 : line;
	int second = lineoffset == 0 ? lastline : lineoffset;
	second = second - len + 1;
	if (errcode >= 4 && errcode <= 6)
		printf("Parse Error line %d offset %d: %s\n", first, 1,
				errstr[errcode]);
	else
		printf("Parse Error line %d offset %d: %s\n", first, second,
				errstr[errcode]);
}
string int2str(int n) {
	stringstream ss;
	ss << n;
	string str = ss.str();
	return str;
}
string PrintOutEW(int rule, string symName, int moduleNum, int symOffset,
		int moduleSize) {
	string res;
	switch (rule) {
	case 2:
		res =
				"Error: This variable is multiple times defined; first value used";
		break;
	case 3:
		res = "Error: " + symName + " is not defined; zero used";
		break;
	case 4:
		res = "Warning: Module " + int2str(moduleNum) + ": " + symName
				+ " was defined but never used\n";
		break;
	case 5:
		res = "Warning: Module " + int2str(moduleNum) + ": " + symName
				+ " to big " + int2str(symOffset) + " (max="
				+ int2str(moduleSize) + ") assume zero relative\n";
		break;
	case 6:
		res =
				"Error: External address exceeds length of uselist; treated as immediate";
		break;
	case 7:
		res = "Warning: Module " + int2str(moduleNum) + ": " + symName
				+ " appeared in the uselist but was not actually used\n";
		break;
	case 8:
		res = "Error: Absolute address exceeds machine size; zero used";
		break;
	case 9:
		res = "Error: Relative address exceeds module size; zero used";
		break;
	case 10:
		res = "Error: Illegal immediate value; treated as 9999";
		break;
	case 11:
		res = "Error: Illegal opcode; treated as 9999";
		break;
	}
	return res;
}
string GetWord() {
	string res = "";
	char c = '\0';
	while (!fin.eof()) {
		fin.get(c);
		if (c == '\n') {
			if (fin.eof())
				line--;
			line++;
//			cout << line << endl;
			lastline = lineoffset;
			lineoffset = 0;
			if (res.length() != 0 && res != "") {
				return res;
			}
		} else if (c == ' ' || c == '\t') {
			lineoffset++;
			if (res.length() != 0 && res != "") {
				return res;
			}
		} else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
				|| (c >= '0' && c <= '9')) {
			lineoffset++;
			res += c;
		}
	}
	return "$";
}
string& replace_all(string& str, const string& old_value,
		const string& new_value) {
	while (true) {
		int pos = 0;
		if ((pos = str.find(old_value, 0)) != string::npos)
			str.replace(pos, old_value.length(), new_value);
		else
			break;
	}
	return str;
}
string ReadDataFromFile(string path) {
	ifstream fin(path.c_str());
	string res;
	string s;
	while (fin >> s) {
		res.append(s);
		res.append(" ");
	}
	return res;
}
string OffsetFormat(int offset) {
	string res;
	if (offset <= 9) {
		res.append("00");
		res.append(int2str(offset));
	} else if (offset <= 99) {
		res.append("0");
		res.append(int2str(offset));
	} else
		res.append(int2str(offset));
	return res;
}
void AddressEntry(int usecount, string s, string r, string useList[]) {
	if (s == "R") {
		if (r.length() >= 5) {
			cout << OffsetFormat(indexAddr) << ": " << "9999 ";
			cout << PrintOutEW(11, "", 0, 0, 0) << endl;
			return;
		}
		string tmp = r.substr(1, 3);
		int address = atoi(tmp.c_str());
		// rule 9

		map<int, int>::iterator it = moduleSize.find(moduleIndex);
//		cout << moduleIndex<<" "<<it->second << endl;
		if (address > it->second) {
			cout << OffsetFormat(indexAddr) << ": " << r.substr(0, 1)
					<< OffsetFormat(offset) << " ";
			cout << PrintOutEW(9, "", 0, 0, 0) << endl;
		} else {
			address += offset;
			cout << OffsetFormat(indexAddr) << ": " << r.substr(0, 1)
					<< OffsetFormat(address) << endl;
		}
	} else if (s == "A") {
		// rule 8
		string tmp = r.substr(1, 3);
		int address = atoi(tmp.c_str());
		if (address > 512) {
			cout << OffsetFormat(indexAddr) << ": " << r.substr(0, 1) << "000 ";
			cout << PrintOutEW(8, "", 0, 0, 0) << endl;
		} else
			cout << OffsetFormat(indexAddr) << ": " << r << endl;
	} else if (s == "I") {
		if (r.length() > 4) {
			cout << OffsetFormat(indexAddr) << ": " << "9999 ";
			cout << PrintOutEW(10, "", 0, 0, 0) << endl;
		} else {
			while (r.length() < 4) {
				r = "0" + r;
			}
			cout << OffsetFormat(indexAddr) << ": " << r << endl;
		}
	} else if (s == "E") {
		string tmp = r.substr(0, 1);
		int optcode = atoi(tmp.c_str());
		tmp = r.substr(1, 3);
		int oprand = atoi(tmp.c_str());
		// rule 6

		if (oprand >= usecount) {
			cout << OffsetFormat(indexAddr) << ": " << r << " ";
			cout << PrintOutEW(6, "", 0, 0, 0) << endl;
		} else {
			// rule 7
			usedOrNot[useList[oprand]] = true;

			map<string, string>::iterator it = m.find(useList[oprand]);

			if (it == m.end()) {
				// rule 3
				cout << OffsetFormat(indexAddr) << ": " << optcode << "000 ";
				cout << PrintOutEW(3, useList[oprand], 0, 0, 0) << endl;
			} else {
				cout << OffsetFormat(indexAddr) << ": " << optcode
						<< OffsetFormat(ReturnNum(it->second)) << endl;
			}
			// rule 7
			map<string, string>::iterator it2 = m.find(useList[oprand]);
			if (it2 == m.end()) {

			} else {
				m[useList[oprand]] = to_string(ReturnNum(m[useList[oprand]]));
			}
		}
	}
}
void CheckSym(string s, int arg = 0) {
	if (s == "$")
		return;
	if (!((s.at(0) >= 'a' && s.at(0) <= 'z')
			|| ((s.at(0) >= 'A' && s.at(0) <= 'Z')))) {
		ParseError(1, s.length() - arg);
		exit(1);
	}
	if (s.length() > 16) {
		ParseError(3, s.length() - arg);
		exit(1);
	}
}
void PassOne() {
	while (!fin.eof()) {
		string s = GetWord();
		if (s == "$")
			break;
		if (s != "" && s.length() != 0
				&& s.find_first_not_of("1234567890") != string::npos) {
			ParseError(0);
			exit(1);
		}
		// definition list
		int defcount = atoi(s.c_str());
		if (defcount > 16) {
			ParseError(4);
			exit(1);
		}
		for (int i = 0; i < defcount; i++) {
			//symbol
			string S = GetWord();
			// check symbol
			CheckSym(S, -1);

			// check parse error
			if (S == "$" || S.find_first_not_of("1234567890") == string::npos) {
				ParseError(1);
				exit(1);
			}
// Relative address
			string tmp = GetWord();

			if (tmp == "" || tmp.length() == 0) {
				ParseError(2);
				exit(1);
			} else if (tmp.find_first_not_of("1234567890") != string::npos) {
				ParseError(0, tmp.length());
				exit(1);
			}

			int R = atoi(tmp.c_str());
			map<string, string>::iterator iter = m.find(S);
			if (iter != m.end()) {
				// rule 2 multiple definition
				m[S] = iter->second + "*";
			} else {
				m[S] = to_string(offset + R) + "/" + to_string(moduleIndex);
			}
		}

		// use list
		s = GetWord();
		int usecount = atoi(s.c_str());
		if (usecount > 16) {
			ParseError(5);
			exit(1);
		}
		for (int i = 0; i < usecount; i++) {
			s = GetWord();
			CheckSym(s);
			if (s.find_first_not_of("1234567890") == string::npos) {
				ParseError(1, s.length());
				exit(1);
			}
		}

		//text
		s = GetWord();
		int codecount = atoi(s.c_str());
		offset += codecount;
		if (offset > 512) {
			ParseError(6);
			exit(0);
		}
		for (int i = 0; i < codecount; i++) {
			s = GetWord();
			if (s != "I" && s != "A" && s != "R" && s != "E") {
				ParseError(2);
				exit(1);
			}
			s = GetWord();
		}

		moduleSize[moduleIndex++] = codecount;
		map<string, string>::iterator it2 = m.begin();
		for (; it2 != m.end(); it2++) {
			int temp = ReturnNum(it2->second);
			if (temp >= offset) {
				cout
						<< PrintOutEW(5, it2->first, moduleIndex - 1, temp,
								codecount - 1);
				m[it2->first] = offset - codecount;
			}
		}
	}
	total = --offset;
	offset = 0;
	moduleIndex = 1;
}

void PassTwo(string path) {
	ifstream fin(path.c_str());
	string s;
	map<string, int> symmap;
	while (fin >> s) {
		// definition list
		int defcount = atoi(s.c_str());
		for (int i = 0; i < defcount; i++) {
			//symbol
			fin >> s;
			fin >> s;
		}

		// use list
		fin >> s;
		int usecount = atoi(s.c_str());
		string* useList = new string[usecount];
		usedOrNot.clear();
		for (int i = 0; i < usecount; i++) {
			fin >> s;
			useList[i] = s;
			usedOrNot[s] = false;
		}

		//text
		fin >> s;
		int codecount = atoi(s.c_str());
		for (int i = 0; i < codecount; i++) {
			fin >> s;
			string S = s;
			fin >> s;
			string R = s;
			AddressEntry(usecount, S, R, useList);
			indexAddr++;
		}
		// rule 7
		map<string, bool>::iterator itt = usedOrNot.begin();
		for (; itt != usedOrNot.end(); ++itt) {
			if (itt->second == false)
				cout << PrintOutEW(7, itt->first, moduleIndex, 0, 0);
		}
		offset += codecount;
		moduleIndex++;
	}
}
void rule4check() {
	int i = 1;
	map<string, string>::iterator it = m.begin();
	for (; it != m.end(); ++it) {
		string::size_type idx = it->second.find("/");
		if (idx != string::npos) {
			if (i == 1) {
				cout << "\n";
				i++;
			}
			cout << PrintOutEW(4, it->first, ReturnNum(it->second.substr(idx+1)), 0, 0);
		}
	}
}
int main(int argc, char* argv[]) {
//	string path = "labsamples/input-1";
	string path = argv[1];
	fin.open(path.c_str());
	PassOne();
	cout << "Symbol Table" << endl;
	map<string, string>::iterator it = m.begin();
	for (; it != m.end(); ++it) {
		cout << it->first << "=" << ReturnNum(it->second);
		string::size_type idx = it->second.find("*");
		if (idx != string::npos) {
			cout << " " << PrintOutEW(2, "", 0, 0, 0);
		}
		cout << endl;
	}
	cout << "\n" << "Memory Map" << endl;
	PassTwo(path);

// rule 4 check
	rule4check();
	return 0;
}

