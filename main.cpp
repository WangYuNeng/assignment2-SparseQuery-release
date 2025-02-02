#include <iostream>
#include <vector>
#include <memory>
#include <chrono>
#include <fstream>
#include <cassert>
#include <map>

using namespace std;

// -------------------------------------------------
// Miscellaneous helper functions 
// -------------------------------------------------
static inline
vector<string> split_at(const string& t, const string& delimiter) {
  string s = t;
  size_t pos = 0;
  std::string token;
  vector<string> tokens;
  while ((pos = s.find(delimiter)) != std::string::npos) {
    token = s.substr(0, pos);
    tokens.push_back(token);
    s.erase(0, pos + delimiter.length());
  }

  tokens.push_back(s);

  return tokens;
}

// -------------------------------------------------
// Type information for table names in the database
// -------------------------------------------------
enum TableName {
  TRADABLE,
  PRICE_OVER_TIME,
  VOLUME_OVER_TIME,
  TRADES
};

// -------------------------------------------------
// Type information for fields in the database 
// -------------------------------------------------
enum FieldType {
  FIELD_TYPE_INT,
  FIELD_TYPE_FLOAT,
  FIELD_TYPE_STRING
};

std::ostream& operator<<(std::ostream& out, const FieldType& tp) {
  if (tp == FIELD_TYPE_STRING) {
    out << "STRING";
  } else if (tp == FIELD_TYPE_INT) {
    out << "INT";
  } else {
    out << "FLOAT";
  }

  return out;
}

// -------------------------------------------------
// The field data structures themselves 
// -------------------------------------------------
class Field {
  public:

    virtual Field* cpy() const = 0;
    virtual FieldType type() const = 0;
    virtual void print(std::ostream& out) const = 0;
    virtual bool equals(const Field& other) const = 0;
};

bool operator==(const Field& a, const Field& b) {
  return a.equals(b);
}

class IntField : public Field {
  public:

    int val;

    IntField(const int& val_) : val(val_) {}

    virtual bool equals(const Field& other) const override {
      if (other.type() != FIELD_TYPE_INT) {
        return false;
      }
      return static_cast<const IntField&>(other).val == val;
    }

    virtual FieldType type() const override { return FIELD_TYPE_INT; }

    virtual void print(std::ostream& out) const override {
      out << val;
    }

    virtual Field* cpy() const override { return new IntField(val); }
};

class FloatField : public Field {
  public:

    float val;

    FloatField(const float& val_) : val(val_) {}

    virtual bool equals(const Field& other) const override {
      if (other.type() != FIELD_TYPE_FLOAT) {
        return false;
      }
      return static_cast<const FloatField&>(other).val == val;
    }

    virtual FieldType type() const override { return FIELD_TYPE_FLOAT; }

    virtual void print(std::ostream& out) const override {
      out << val;
    }

    virtual Field* cpy() const override { return new FloatField(val); }
};

class StringField : public Field {
  public:

    std::string val;

    StringField(const std::string& val_) : val(val_) {}

    virtual bool equals(const Field& other) const override {
      if (other.type() != FIELD_TYPE_STRING) {
        return false;
      }
      return static_cast<const StringField&>(other).val == val;
    }

    virtual FieldType type() const override { return FIELD_TYPE_STRING; }

    virtual void print(std::ostream& out) const override {
      out << val;
    }

    virtual Field* cpy() const override { return new StringField(val); }
};

std::ostream& operator<<(std::ostream& out, const Field& f) {
  f.print(out);
  return out;
}

// -------------------------------------------------
// Helper class for representing rows of fields 
// -------------------------------------------------
class Record {

  public:

    map<string, Field*> fields;

    float floatAt(const std::string& name) const {

      if (!(fields.find(name) != end(fields))) {
        cout << "Error: No such field as " << name << " in record, which has fields" << endl;
        for (auto r : fields) {
          cout << "\t" << r.first << " -> " << *(r.second) << endl;
        }
      }
      assert(fields.find(name) != end(fields));

      auto f = fields.at(name);
      assert(f->type() == FIELD_TYPE_FLOAT);
      return static_cast<FloatField*>(f)->val;
    }

    string stringAt(const std::string& name) const {
      assert(fields.find(name) != end(fields));

      auto f = fields.at(name);
      assert(f->type() == FIELD_TYPE_STRING);
      return static_cast<StringField*>(f)->val;
    }

    int intAt(const std::string& name) const {
      assert(fields.find(name) != end(fields));

      auto f = fields.at(name);
      assert(f->type() == FIELD_TYPE_INT);
      return static_cast<IntField*>(f)->val;
    }
};

// -------------------------------------------------
// Abstract class for a table 
// -------------------------------------------------
class Table {
  public:


    virtual void addRecord(std::vector<unique_ptr<Field> >& r) = 0;

    virtual int numColumns() const = 0;

    virtual std::string getName() const = 0;

    virtual FieldType fieldType(const int columnNum) const = 0;
    virtual std::string fieldName(const int columnNum) const = 0;
    virtual void print(std::ostream& out) const = 0;
};

std::ostream& operator<<(std::ostream& out, const Table& t) {
  t.print(out);
  return out;
}

// -------------------------------------------------
// An inefficient, but usable table implementation 
// -------------------------------------------------
class DenseTable : public Table {

  public:

    std::string name;
    std::vector<string> columnNames;
    std::vector<FieldType> columnTypes;
    std::vector<std::vector<unique_ptr<Field> > > records;

    DenseTable(const std::string& name_,
        const std::vector<string>& columnNames_,
        const std::vector<FieldType>& columnTypes_) :
      name(name_), columnNames(columnNames_), columnTypes(columnTypes_) {
        assert(columnNames.size() == columnTypes.size());
      }

    int columnIndex(const std::string& name) const {
      for (int i = 0; i < (int) columnNames.size(); i++) {
        if (columnNames.at(i) == name) {
          return i;
        }
      }
      assert(false);
    }

    FieldType columnType(const std::string& name) const {
      for (int i = 0; i < (int) columnNames.size(); i++) {
        if (columnNames.at(i) == name) {
          return columnTypes.at(i);
        }
      }
      cout << "Error: No column named: " << name << " in..." << endl;
      cout << *this << endl;
      assert(false);
    }

    virtual std::string fieldName(const int columnNum) const override {
      assert(columnNum < (int) columnNames.size());
      return columnNames.at(columnNum);
    }

    virtual FieldType fieldType(const int columnNum) const override {
      assert(columnNum < (int) columnTypes.size());
      return columnTypes.at(columnNum);
    }

    virtual std::string getName() const override {
      return name;
    }

    virtual void addRecord(std::vector<unique_ptr<Field> >& r) override {
      records.push_back(std::move(r));
    }

    const std::vector<std::vector<unique_ptr<Field> > >& rows() const { return records; }

    virtual int numColumns() const override {
      return columnNames.size();
    }

    virtual void print(std::ostream& out) const override {

      out << "<TABLE>," << getName() << endl;
      for (int i = 0; i < numColumns(); i++) {
        if (i > 0) {
          out << ",";
        }
        out << fieldType(i);
      }
      out << endl;
      for (int i = 0; i < numColumns(); i++) {
        if (i > 0) {
          out << ",";
        }
        out << fieldName(i);
      }
      out << endl;
      for (auto& row : rows()) {
        int i = 0;
        for (auto& field : row) {
          if (i > 0) {
            out << ",";
          }
          out << *field;
          i++;
        }
        out << endl;
      }
    }

};


// -------------------------------------------------
// Abstract class that represents a data structure
// to store tables and execute queries
// -------------------------------------------------
class QueryEngine {
  public:

    virtual void loadTablesFromCSV(const std::vector<vector<string> >& lines) = 0;
    virtual std::unique_ptr<Table> exe() = 0;
};

// -------------------------------------------------
// Specific query engine implementation that
// uses the DenseTable
// -------------------------------------------------
class ReferenceQueryEngine : public QueryEngine {
  public:

    vector<DenseTable> tables;

    // ---------------------------------------------------------------------------------
    // Some notes on my tables:
    // I believe the most efficient way is to perform the query with parsing and reture 
    // stored results when exe(). But that sounds not the purpose of this assignment.
    // Here's some bottomlines which I think is reasonable for the query engine. 
    // 1. Datastructures contain the same amount of information with DenseTable, i.e. can 
    // tranform from one to the other and vice versa. 
    // 2. Only save data in loadTablesFromCSV and do not perform any query-related computation.
    // ---------------------------------------------------------------------------------
    vector<tuple<std::string, vector<std::string>, vector<FieldType>>> table_headers;
    map<std::string, std::string> name_to_class;
    map<std::string, map<int, float>> name_to_date_price, name_to_date_volume;
    map<std::string, vector<tuple<int, int, int>>> name_to_trades;

    virtual void loadTablesFromCSV(const std::vector<vector<string> >& lines) {

      std::string cur_table;
      int cur_table_flag;

      for (int i = 0; i < (int) lines.size(); i++)  {
        auto l = lines.at(i);
        assert(l.size() > 0);
        if (l.at(0) == "<TABLE>") {

          assert(i < (int) (lines.size()) - 2);

          auto columnNames = lines.at(i + 1);

          assert(columnNames.size() >= 1);

          auto columnTypeNames = lines.at(i + 2);

          assert(columnNames.size() == columnTypeNames.size());

          vector<FieldType> columnTypes;
          for (auto c : columnTypeNames) {
            if (c == "STRING") {
              columnTypes.push_back(FIELD_TYPE_STRING);
            } else if (c == "INT") {
              columnTypes.push_back(FIELD_TYPE_INT);
            } else if (c == "FLOAT") {
              columnTypes.push_back(FIELD_TYPE_FLOAT);
            } else {
              cout << "Error: Unrecognized column type: " << c << endl;
              assert(false);
            }
          }
          tables.push_back(DenseTable(l.at(1), columnNames, columnTypes));

          cur_table = l.at(1);
          table_headers.push_back(make_tuple(cur_table, columnNames, columnTypes));
          if (cur_table == "tradable") {
            cur_table_flag = TRADABLE;
          } else if (cur_table == "price-over-time") {
            cur_table_flag = PRICE_OVER_TIME;
          } else if (cur_table == "volume-over-time") {
            cur_table_flag = VOLUME_OVER_TIME;
          } else if (cur_table == "trades") {
            cur_table_flag = TRADES;
          } // else assert(false);

          i += 2;
        } else {
          assert(tables.size() > 0);

          DenseTable& currentTable = tables.back();
          int numCols = currentTable.numColumns();
          assert(numCols == l.size());

          vector<unique_ptr<Field> > record;
          for (int c = 0; c < numCols; c++) {
            FieldType tp = currentTable.fieldType(c);
            if (tp == FIELD_TYPE_STRING) {
              record.push_back(unique_ptr<Field>(new StringField(l.at(c))));
            } else if (tp == FIELD_TYPE_INT) {
              record.push_back(unique_ptr<Field>(new IntField(stoi(l.at(c)))));
            } else if (tp == FIELD_TYPE_FLOAT) {
              record.push_back(unique_ptr<Field>(new FloatField(stof(l.at(c)))));
            } else {
              cout << "Unreconized field type: " << tp << endl;
              assert(false);
            }
          }

          std::string name, asset_class;
          int id, day, quant;
          float price, volume;
          switch (cur_table_flag)
          {
          case TRADABLE: {
            name = static_cast<StringField*>(record[0].get())->val;
            asset_class = static_cast<StringField*>(record[1].get())->val;
            name_to_class.insert({name, asset_class});
            break;
          }
          case PRICE_OVER_TIME: {
            name = static_cast<StringField*>(record[1].get())->val;
            day = static_cast<IntField*>(record[0].get())->val;
            price = static_cast<FloatField*>(record[2].get())->val;
            auto search = name_to_date_price.find(name);
            if (search != name_to_date_price.end()) {
              (search->second).insert({day, price});
            } else {
              auto price_map = map<int, float>{{day, price}};
              name_to_date_price.insert({name, price_map});
            }
            break;
          }
          case VOLUME_OVER_TIME: {
            name = static_cast<StringField*>(record[1].get())->val;
            day = static_cast<IntField*>(record[0].get())->val;
            volume = static_cast<FloatField*>(record[2].get())->val;
            auto search = name_to_date_volume.find(name);
            if (search != name_to_date_volume.end()) {
              (search->second).insert({day, volume});
            } else {
              auto volume_map = map<int, float>{{day, volume}};
              name_to_date_volume.insert({name, volume_map});
            }
            break;
          }
          case TRADES: {
            name = static_cast<StringField*>(record[2].get())->val;
            day = static_cast<IntField*>(record[1].get())->val;
            id = static_cast<IntField*>(record[0].get())->val;
            quant = static_cast<IntField*>(record[3].get())->val;
            auto search = name_to_trades.find(name);
            if (search != name_to_trades.end()) {
              (search->second).push_back(make_tuple(id, day, quant));
            } else {
              auto vec = vector<tuple<int, int, int>>{make_tuple(id, day, quant)};
              name_to_trades.insert({name, vec});
            }
            break;
          }
          default:
            break; // assert(false)
          }
          currentTable.addRecord(record);
        }
      }

    }

    virtual std::unique_ptr<Table> exe() {
      // STUDENTS: FILL IN THIS FUNCTION

      auto valid_stock_cnt = 0, valid_bond_cnt = 0;
      auto valid_flag = false;
      for (auto ite = name_to_class.begin(); ite != name_to_class.end(); ++ite) {
        if (ite->second != "stock" && ite->second != "bond" ) {
          continue;
        }
        auto& name = ite->first;

        auto trades_ite = name_to_trades.find(name);
        if (trades_ite == name_to_trades.end())
          continue;
        
        auto search = name_to_date_price.find(name);
        if (search != name_to_date_price.end()) {
          auto& date_to_price = search->second;
          valid_flag = true;
          for (auto ite2 = date_to_price.lower_bound(13); ite2 != date_to_price.end() && ite2->first <= 268; ++ite2) {
            if (ite2->second > 299.0) {
              valid_flag = false;
              break;
            }
          }
        }
        if (valid_flag == true) {
          if (ite->second == "stock") valid_stock_cnt += trades_ite->second.size();
          else valid_bond_cnt += trades_ite->second.size();
          continue;
        } 
        search = name_to_date_volume.find(name);
        if (search != name_to_date_volume.end()) {
          auto& date_to_volume = search->second;
          valid_flag = true;
          for (auto ite2 = date_to_volume.lower_bound(13); ite2 != date_to_volume.end() && ite2->first <= 268; ++ite2) {
            if (ite2->second < 10.0) {
              valid_flag = false;
              break;
            }
          }
        }
        if (valid_flag == true) {
          if (ite->second == "stock") valid_stock_cnt += trades_ite->second.size();
          else valid_bond_cnt += trades_ite->second.size();
        } 
      }

      auto ret_table = new DenseTable(string("asset-class_counts"), {string("asset-class"), 
                            string("count")}, {FIELD_TYPE_STRING, FIELD_TYPE_INT});
      vector<unique_ptr<Field> > record;
      if (valid_bond_cnt != 0) {
        record.push_back(unique_ptr<Field>(new StringField("bond")));
        record.push_back(unique_ptr<Field>(new IntField(valid_bond_cnt)));
        ret_table->addRecord(record);
        record.clear();
      }
      if (valid_stock_cnt != 0) {
        record.push_back(unique_ptr<Field>(new StringField("stock")));
        record.push_back(unique_ptr<Field>(new IntField(valid_stock_cnt)));
        ret_table->addRecord(record);
      }
      return unique_ptr<Table>(ret_table);
    }
};

// -------------------------------------------------
// The driver function 
// -------------------------------------------------
int main(const int argc, const char** argv) {
  if (argc != 2) {
    cout << "Error: Usage: ./fakedb <input_tables_file>" << endl;
    return -1;
  }

  string tableFile = argv[1];

  std::ifstream t(tableFile);
  std::string str((std::istreambuf_iterator<char>(t)),
      std::istreambuf_iterator<char>());

  ReferenceQueryEngine engine;

  vector<string> lines = split_at(str, "\n");
  lines.pop_back();
  vector<vector<string> > csvLines;
  for (auto l : lines) {
    csvLines.push_back(split_at(l, ","));
  }

  cout << "Input table file " << tableFile << " has " << lines.size() << " lines" << endl;

  // Load the tables for the query
  engine.loadTablesFromCSV(csvLines);

  // Run and time the query using several runs to remove
  // cold-start overhead and noise
  double min_time = 1e10;

  unique_ptr<Table> table;
  for (int i = 0; i < 5; i++) {
    double total_elapsed = 0.;

    auto start = std::chrono::system_clock::now();
    table = engine.exe();
    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    total_elapsed += elapsed.count();
    if (total_elapsed < min_time) {
      min_time = total_elapsed;
    }
  }

  assert(table != nullptr);
  std::cout << "Result:" << endl;
  cout << *table << endl;

  // Uncomment this line to see the timing information for your code
  // std::cout << "Query Runtime: " << min_time << " seconds" << std::endl;
  return 0;
}
