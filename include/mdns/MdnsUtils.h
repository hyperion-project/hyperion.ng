#ifndef MDNS_UTILS_H
#define MDNS_UTILS_H

#include <QString>

///
/// @brief Lightweight mDNS name-check utilities.
///        These functions only inspect the string and do not depend on any
///        mDNS library, so they can be used regardless of ENABLE_MDNS.
///
namespace MdnsUtils {

///
/// @brief Check if the passed name is an mDNS service- or hostname
/// @param[in]  mdnsName  The name to be checked
/// @return     True when the name ends with ".local" or ".local."
///
inline bool isMdns(const QString& mdnsName)
{
	return mdnsName.endsWith(".local") || mdnsName.endsWith(".local.");
}

///
/// @brief Check if the passed name is an mDNS service instance name
/// @param[in]  mdnsServiceName  The name to be checked
/// @return     True when the name ends with "._tcp.local" or "._tcp.local."
///
inline bool isMdnsService(const QString& mdnsServiceName)
{
	return mdnsServiceName.endsWith("._tcp.local") || mdnsServiceName.endsWith("._tcp.local.");
}

} // namespace MdnsUtils

#endif // MDNS_UTILS_H
