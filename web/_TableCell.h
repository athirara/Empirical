//  This file is part of Empirical, https://github.com/devosoft/Empirical
//  Copyright (C) Michigan State University, 2017.
//  Released under the MIT Software license; see doc/LICENSE
//
//
//  The TableCell widget, which behaves like the Table widget, but focuses on a single cell.
//
//  DO NOT include directly.  All files begining with '_' are for internal use only.

class TableCell : public Table {
public:
  TableCell(size_t r, size_t c, const std::string & in_id="") : Table(r,c,in_id) { ; }
  TableCell(const Table & in) : Table(in) { ; }
  TableCell(const Widget & in) : Table(in) { ; }
  TableCell(internal::TableInfo * in_info, size_t _row=0, size_t _col=0)
    : Table(in_info, _row, _col) { ; }

  void DoCSS(const std::string & setting, const std::string & value) override {
    Info()->rows[cur_row].data[cur_col].extras.style.Set(setting, value);
    if (IsActive()) Info()->ReplaceHTML();   // @CAO only should replace cell's CSS
  }

  void DoAttr(const std::string & setting, const std::string & value) override {
    Info()->rows[cur_row].data[cur_col].extras.attr.Set(setting, value);
    if (IsActive()) Info()->ReplaceHTML();   // @CAO only should replace cell's CSS
  }

  void DoListen(const std::string & event_name, size_t fun_id) override {
    Info()->rows[cur_row].data[cur_col].extras.listen.Set(event_name, fun_id);
    if (IsActive()) Info()->ReplaceHTML();   // @CAO only should replace cell's CSS
  }

  TableCell & Clear() { Info()->ClearCell(cur_row, cur_col); return *this; }
  TableCell & ClearStyle() { Info()->ClearCellStyle(cur_row, cur_col); return *this; }
  TableCell & ClearChildren() { Info()->ClearCellChildren(cur_row, cur_col); return *this; }
  TableCell & ClearCells() { Info()->ClearCell(cur_row, cur_col); return *this; }

  std::string GetCSS(const std::string & setting) override {
    return Info()->rows[cur_row].data[cur_col].extras.GetStyle(setting);
  }

  TableCell & SetHeader(bool _h=true) {
    Info()->rows[cur_row].data[cur_col].header = _h;
    if (IsActive()) Info()->ReplaceHTML();   // @CAO only should replace cell's CSS
    return *this;
  }

  // Allow the row and column span of the current cell to be adjusted.
  TableCell & SetRowSpan(size_t new_span) {
    emp_assert((cur_row + new_span <= GetNumRows()) && "Row span too wide for table!");

    auto & datum = Info()->rows[cur_row].data[cur_col];
    const size_t old_span = datum.rowspan;
    const size_t col_span = datum.colspan;
    datum.rowspan = new_span;

    // For each col, make sure NEW rows are masked!
    for (size_t row = cur_row + old_span; row < cur_row + new_span; row++) {
      for (size_t col = cur_col; col < cur_col + col_span; col++) {
        Info()->rows[row].data[col].masked = true;
      }
    }

    // For each row, make sure former columns are unmasked!
    for (size_t row = cur_row + new_span; row < cur_row + old_span; row++) {
      for (size_t col = cur_col; col < cur_col + col_span; col++) {
        Info()->rows[row].data[col].masked = false;
      }
    }

    // Redraw the entire table to fix row span information.
    if (IsActive()) Info()->ReplaceHTML();

    return *this;
  }

  TableCell & SetColSpan(size_t new_span) {
    emp_assert((cur_col + new_span <= GetNumCols()) && "Col span too wide for table!",
               cur_col, new_span, GetNumCols(), GetID());

    auto & datum = Info()->rows[cur_row].data[cur_col];
    const size_t old_span = datum.colspan;
    const size_t row_span = datum.rowspan;
    datum.colspan = new_span;

    // For each row, make sure new columns are masked!
    for (size_t row = cur_row; row < cur_row + row_span; row++) {
      for (size_t col = cur_col + old_span; col < cur_col + new_span; col++) {
        Info()->rows[row].data[col].masked = true;
      }
    }

    // For each row, make sure former columns are unmasked!
    for (size_t row = cur_row; row < cur_row + row_span; row++) {
      for (size_t col = cur_col + new_span; col < cur_col + old_span; col++) {
        Info()->rows[row].data[col].masked = false;
      }
    }

    // Redraw the entire table to fix col span information.
    if (IsActive()) Info()->ReplaceHTML();

    return *this;
  }

  TableCell & SetSpan(size_t row_span, size_t col_span) {
    // @CAO Can do this more efficiently, but probably not worth it.
    SetRowSpan(row_span);
    SetColSpan(col_span);
    return *this;
  }

};
