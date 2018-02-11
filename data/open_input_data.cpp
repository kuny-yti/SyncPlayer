#include "open_input_data.h"

open_input_data::open_input_data() :
    dev_request(), urls()
{
}

void open_input_data::save(QDataStream &os) const
{
    dev_request.save(os);
    os << urls;
}

void open_input_data::load(QDataStream &is)
{
    dev_request.load(is);
    is >> urls;
}
