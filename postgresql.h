#ifndef POSTGRESQL_H_
#define POSTGRESQL_H_ 1

#include <postgresql/libpq-fe.h>

struct postgresql
{
  PGconn *pg;

  struct result
  {
    PGresult *pgresult;

    result(PGresult *pgresult);
    ~result();

    size_t affected_row_count() const;
    size_t row_count() const;

    const char *operator()(size_t row, size_t column) const;

    bool operator!() const;
  };

  postgresql(const char *connect_string);
  ~postgresql();

  result query(const char *format, ...);

  const char *last_error();
};

#endif /* !POSTGRESQL_H_ */
