#ifndef _TSD_PMT_SECTION_FILTER_IMPL_HPP_
#define _TSD_PMT_SECTION_FILTER_IMPL_HPP_

#include "context.hpp"
#include "section.hpp"
#include "pmt.hpp"
#include "filter/pes_filter.hpp"

namespace tsd
{

void pmt_section_filter::do_handle_section(
    context& c,
    const char* section_buffer,
    size_t section_length) {
  section s;
  s.unpack(section_buffer, section_length);
  program_map_table pmt;
  s.convert(pmt);

  c.get_view().print(
      c.get_packet_num(),
      s.header,
      pmt,
      last_version_ != s.header.version);

  if(last_version_ == s.header.version)
    return ;

  if(!c.is_opened(pmt.pcr_pid)) {
    c.open_pcr_filter(pmt.pcr_pid);
  }

  for(auto& i : c.pat->association) {
    if(i.program_number != 0) {
      if(i.program_number == s.header.table_id_extension) {
        c.signal_pmt();
      }
      break;
    }
  }

  c.program_pcr[s.header.table_id_extension] = pmt.pcr_pid;

  //for(auto& pe : pmt.program_elements) {
  //  if(pe.stream_type == 0x02) {
  //    // video
  //    if(!c.is_opened(pe.elementary_pid)) {
  //      c.open_pes_filter(
  //          pe.elementary_pid,
  //          std::unique_ptr<pes_filter>(
  //            new pes_filter()));
  //    }
  //  }
  //}

  last_version_ = s.header.version;
}

}


#endif
