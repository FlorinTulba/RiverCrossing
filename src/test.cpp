#include <limits>
#include <cmath>
#include <iostream>
#include <memory>

#include <boost/optional/optional.hpp>

using namespace std;

template<typename Type>
class AbsExpr /*abstract*/ {

    #ifdef UNIT_TESTING // leave fields public for Unit tests
public:
    #else // keep fields protected otherwise
protected:
    #endif

  boost::optional<Type> val; ///< cached result of the expression if possible

public:
  virtual ~AbsExpr()/* = 0*/ {}

  /// Provide the cached result of the expression when this is a constant
  const boost::optional<Type>& constValue() const {return val;}
};

/// Base of numeric expression types
struct NumericExpr /*abstract*/ : AbsExpr<double> {};

class ValueOrRange {

    #ifdef UNIT_TESTING // leave fields public for Unit tests
public:
    #else // keep fields protected otherwise
protected:
    #endif

  union {
    std::shared_ptr<const NumericExpr> _value;  ///< either the value
    std::shared_ptr<const NumericExpr> _from;   ///< or the inferior range limit
  };
  std::shared_ptr<const NumericExpr> _to; ///< the superior range limit (for a range)

  /// @throw logic_error for NaN values
  static void validateDouble(double d);

  /// @throw logic_error for out of order values
  static void validateRange(double a, double b);

  /// @throw logic_error for NULL / NaN values or for out of order values
  void validate() const;

public:
  ~ValueOrRange();

  /// Ctor for the value of a numeric expression
  ValueOrRange(const std::shared_ptr<const NumericExpr> &value_);

  /// Ctor for a range provided as 2 numeric expressions
  ValueOrRange(const std::shared_ptr<const NumericExpr> &from_,
      const std::shared_ptr<const NumericExpr> &to_);
  ValueOrRange(const ValueOrRange &other);
  
  ValueOrRange(ValueOrRange&&) = delete;
  void operator=(const ValueOrRange &other) = delete;
  void operator=(ValueOrRange &&other) = delete;

  /// @return true for range; false for value
  bool isRange() const;

  /**
    @param st the symbols' table
    @return the value based on the data from the symbols' table
    @throw logic_error for ranges or for NaN values
    @throw out_of_range when referring symbols missing from the table
  */
  double value(const SymbolsTable &st) const;
};


int main() {
    const boost::optional<double> d = numeric_limits<double>::quiet_NaN();
    cout<<isnan(*d)<<endl;
}