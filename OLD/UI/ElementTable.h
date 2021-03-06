#ifndef EMP_UI_ELEMENT_TABLE_H
#define EMP_UI_ELEMENT_TABLE_H

///////////////////////////////////////////////////////////////////////////////////////////
//
//  Manage a section of the current web page
//

#include <functional>
#include <string>
#include <vector>

#include <emscripten.h>

#include "../tools/DynamicStringSet.h"

#include "Element.h"

namespace emp {
namespace UI {

  class ElementTable : public Element, public Table {
  private:
    void UpdateHTML() override {
      HTML.str("");       // Clear the current HTML
      HTML << "<table id=\"" << div_id << obj_ext << "\">";

      // Loop through all of the rows in the table.
      for (auto & row : rows) {
        HTML << "<tr id=\"" << row.GetDivID() << row.GetObjExt() << "\">";
        //HTML << "<tr>";

        // Loop through each cell in this row.
        for (auto & datum : row.GetCells()) {
          // If this cell is masked by another, skip it!
          if (datum.IsMasked()) continue;

          // Print opening tag.
          HTML << (datum.IsHeader() ? "<th id=" : "<td id=\"")
               << datum.GetDivID() << datum.GetObjExt()
               << "\"";
          // HTML << (datum.IsHeader() ? "<th" : "<td");

          // If this cell spans multiple rows or columns, indicate!
          if (datum.GetColSpan() > 1) HTML << " colspan=\"" << datum.GetColSpan() << "\"";
          if (datum.GetRowSpan() > 1) HTML << " rowspan=\"" << datum.GetRowSpan() << "\"";
          
          HTML << ">";
          
          // If this cell has contents, print them!
          if (datum.GetChildID() >= 0) {
            Element * element = children[datum.GetChildID()];
            const std::string & tag = element->GetWrapperTag();
            HTML << "<" << tag << " id=\"" << element->GetName() << "\"></" << tag << ">\n";
          }

          // Print closing tag.
          HTML << (datum.IsHeader() ? "</th>" : "</td>");
        }

        HTML << "</tr>";
      }

      HTML << "</table>";
    }
    void UpdateCSS() override {
      Table::UpdateCSS();
    }

    Element & GetCurSlate() {
      // Determine if the current cell has a slate already.
      auto & cur_cell = GetCurCell();
      int child_id = cur_cell.GetChildID();

      // If not, generate one (and let the cell know).
      if (child_id < 0) {
        child_id = (int) children.size();
        std::string child_name = cur_cell.GetDivID() + "s";
        children.push_back( BuildElement(Slate(child_name), this) );
        cur_cell.SetChildID(child_id);
      }

      // Return the element, now that we know we have it.
      return *(children[child_id]);
    }

    // When appending children to a table, forward to an internal slate.
    Element & Append(const std::string & in_text) override { return GetCurSlate() << in_text; }
    Element & Append(const std::function<std::string()> & fun) override { return GetCurSlate() << fun; }
    Element & Append(emp::UI::Button info) override { return GetCurSlate() << info; }
    Element & Append(emp::UI::Canvas info) override { return GetCurSlate() << info; }
    Element & Append(emp::UI::Image info) override { return GetCurSlate() << info; }
    Element & Append(emp::UI::Table info) override { return GetCurSlate() << info; }
    Element & Append(emp::UI::Text info) override { return GetCurSlate() << info; }
    Element & Append(emp::UI::Slate info) override { return GetCurSlate() << info; }

    Element & Append(const emp::UI::GetCell & cell) override {
      this->GetCell(cell.row, cell.col);
      if (cell.row_span > 0) SetRowSpan(cell.row_span);
      if (cell.col_span > 0) SetColSpan(cell.col_span);
      return *this;
    }

    Element & Append(const emp::UI::GetRow & row) override {
      this->GetRow(row.row);
      return *this;
    }

  protected:
    ElementTable(const ElementTable & src, Element * parent, const std::string & ext)
      : Element(src, parent, ext), Table(src) { ; }

  public:
    ElementTable(const Table & in_table, Element * in_parent)
      : Element(in_table.GetDivID(), in_parent), Table(in_table) { ; }
    ~ElementTable() { ; }

    virtual Element * Clone(Element * parent, const std::string & ext) const override {
      return new ElementTable(*this, parent, ext);
    };

    virtual bool IsTable() const override { return true; }

    // Mask some "Get" methods to facilitate appending (have them return ElementTable type)
    ElementTable & GetCell(int r, int c) {  Table::GetCell(r,c);  return (ElementTable &) *this; }
    ElementTable & GetRow(int r) { Table::GetRow(r);  return (ElementTable &) *this; }
    ElementTable & GetTable() { Table::GetTable(); return (ElementTable &) *this; }
    

    // Allow the row and column span of the current cell to be adjusted.
    ElementTable & ColSpan(int new_span) {
      SetColSpan(new_span);
      return (ElementTable &) *this;
    }

    ElementTable & RowSpan(int new_span) {
      SetRowSpan(new_span);
      return (ElementTable &) *this;
    }
    
    virtual std::string GetType() override {
      return "ElementTable";
    }

    virtual bool OK(std::stringstream & ss, bool verbose=false, const std::string & prefix="") override {
      bool ok = true;

      if (verbose) {
        ss << prefix << "Scanning: emp::UI::ElementTable with name = '" << name << "'" << std::endl;
      }

      Element::OK(ss, verbose, prefix);  // Check base Element class; same obj, don't change prefix
      Table::OK(ss, verbose, prefix);    // Check base Table class.

      return ok;
    }

  };

};
};

#endif
