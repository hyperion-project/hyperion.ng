#ifndef VERSION_H
#define VERSION_H

/**
 * Semver - The Semantic Versioning, https://github.com/euskadi31/semver-cpp
 *
 * Copyright (c) 2013 Axel Etcheverry, 2021 enhancements & fixes Lord-Grey
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

namespace semver {

    enum PRE_RELEASE {
        PRE_RELEASE_ALPHA,
        PRE_RELEASE_BETA,
        PRE_RELEASE_RC,
        PRE_RELEASE_NONE
    };

    typedef enum PRE_RELEASE pre_release_t;

    class version
    {
    private:
        std::string m_version;
        int m_major;
        int m_minor;
        int m_patch;
        pre_release_t m_pre_release_type;
        std::string m_pre_release_id;
        std::string m_pre_release;
        std::string m_build;
        bool m_is_valid;
        bool m_is_stable;

        enum m_type {
            TYPE_MAJOR,
            TYPE_MINOR,
            TYPE_PATCH,
            TYPE_PRE_RELEASE,
            TYPE_PRE_RELEASE_ID,
            TYPE_BUILD
        };

        void parse()
        {
            int type = TYPE_MAJOR;

            std::string major, minor, patch;

            for (std::size_t i = 0; i < m_version.length(); i++)
            {
                char chr = m_version[i];
                int chr_dec = chr;

                switch (type)
                {
                    case TYPE_MAJOR:

                        if (chr == '.')
                        {
                            type = TYPE_MINOR;
                            continue;
                        }

                        if (chr_dec < 48 || chr_dec > 57)
                        {
                            m_is_valid = false;
                        }

                        // major
                        major += chr;
                        break;

                    case TYPE_MINOR:

                        if (chr == '.')
                        {
                            type = TYPE_PATCH;
                            continue;
                        }

                        if (chr_dec < 48 || chr_dec > 57)
                        {
                            m_is_valid = false;
                        }

                        minor += chr;

                        break;

                    case TYPE_PATCH:

                        if (chr == '-')
                        {
                            type = TYPE_PRE_RELEASE;
                            continue;
                        }

                        if (chr == '+')
                        {
                            type = TYPE_BUILD;
                            continue;
                        }


                        if (chr_dec < 48 || chr_dec > 57)
                        {
                            m_is_valid = false;
                        }

                        patch += chr;

                        break;

                    case TYPE_PRE_RELEASE:

                        if (chr == '.')
                        {
                            type = TYPE_PRE_RELEASE_ID;
                            m_pre_release += chr;
                            continue;
                        }

                        if (chr == '+')
                        {
                            type = TYPE_BUILD;
                            continue;
                        }

                        if (
                            (chr_dec < 48 || chr_dec > 57) && // 0-9
                            (chr_dec < 65 || chr_dec > 90) && // A-Z
                            (chr_dec < 97 || chr_dec > 122) && // a-z
                            (chr_dec != 45) && // -
                            (chr_dec != 46) // .
                        )
                        {
                            m_is_valid = false;
                        }

                        m_pre_release += chr;
                        break;

                    case TYPE_PRE_RELEASE_ID:

                        if (chr == '+')
                        {
                            type = TYPE_BUILD;
                            continue;
                        }

                        if (
                            (chr_dec < 48 || chr_dec > 57) && // 0-9
                            (chr_dec < 65 || chr_dec > 90) && // A-Z
                            (chr_dec < 97 || chr_dec > 122) && // a-z
                            (chr_dec != 45) // -
                        )
                        {
                            m_is_valid = false;
                        }

                        m_pre_release += chr;
                        m_pre_release_id += chr;
                        break;

                    case TYPE_BUILD:

                        if (
                            (chr_dec < 48 || chr_dec > 57) && // 0-9
                            (chr_dec < 65 || chr_dec > 90) && // A-Z
                            (chr_dec < 97 || chr_dec > 122) && // a-z
                            (chr_dec != 45) // -
                        )
                        {
                            m_is_valid = false;
                        }

                        m_build += chr;
                        break;
                }
            }

            if (m_is_valid)
            {
                std::istringstream(major) >> m_major;
                std::istringstream(minor) >> m_minor;
                std::istringstream(patch) >> m_patch;

                if (m_pre_release.empty())
                {
                    m_pre_release_type = PRE_RELEASE_NONE;
                }
                else if (m_pre_release.find("alpha") != std::string::npos)
                {
                    m_pre_release_type = PRE_RELEASE_ALPHA;
                } 
                else if (m_pre_release.find("beta") != std::string::npos)
                {
                    m_pre_release_type = PRE_RELEASE_BETA;
                }
                else if (m_pre_release.find("rc") != std::string::npos)
                {
                    m_pre_release_type = PRE_RELEASE_RC;
                }

                if (m_major == 0 && m_minor == 0 && m_patch == 0)
                {
                    m_is_valid = false;
                }

                if (!m_pre_release_id.empty() && m_pre_release_id[0] == '0')
                {
                    m_is_valid = false;
                }

                if (m_major == 0)
                {
                    m_is_stable = false;
                }

                if (m_pre_release_type != PRE_RELEASE_NONE)
                {
                    m_is_stable = false;
                }
            }
        }

    public:
        /**
         * Parse the version string
         */
        version(const std::string& version)
        {
			setVersion(version);
        }

		version(const version&) = default;

		~version() = default;

		/**
		 * Set version
		 */
		bool setVersion(const std::string& version)
		{
			m_version           = version;
			m_major             = 0;
			m_minor             = 0;
			m_patch             = 0;
			m_build             = "";
			m_pre_release       = "";
			m_pre_release_id    = "";
			m_is_stable         = true;

			if (version.empty())
			{
				m_is_valid = false;
			}
			else
			{
				m_is_valid = true;

				parse();
			}
			return m_is_valid;
		}

        /**
         * Get full version
         */
        const std::string& getVersion() const
        {
            return m_version;
        }

        /**
         * Get the major of the version
         */
        const int& getMajor() const
        {
            return m_major;
        }

        /**
         * Get the minor of the version
         */
        const int& getMinor() const
        {
            return m_minor;
        }

        /**
         * Get the patch of the version
         */
        const int& getPatch() const
        {
            return m_patch;
        }

        /**
         * Get the build of the version
         */
        const std::string& getBuild() const
        {
            return m_build;
        }

        /**
         * Get the release type of the version
         */
        const pre_release_t& getPreReleaseType() const
        {
            return m_pre_release_type;
        }

        /**
         * Get the release identifier of the version
         */
        const std::string& getPreReleaseId() const
        {
            return m_pre_release_id;
        }

        /**
         * Get the release of the version
         */
        const std::string& getPreRelease() const
        {
            return m_pre_release;
        }

        /**
         * Check if the version is stable
         */
        const bool& isStable() const
        {
            return m_is_stable;
        }

        /**
         * Check if the version is valid
         */
        const bool& isValid() const
        {
            return m_is_valid;
        }


        int compare(version& rgt)
        {

            if ((*this) == rgt)
            {
                return 0;
            }

            if ((*this) > rgt)
            {
                return 1;
            }

            return -1;
        }

        version& operator= (version& rgt)
        {
            if ((*this) != rgt)
            {
                this->m_version             = rgt.getVersion();
                this->m_major               = rgt.getMajor();
                this->m_minor               = rgt.getMinor();
                this->m_patch               = rgt.getPatch();
                this->m_pre_release_type    = rgt.getPreReleaseType();
                this->m_pre_release_id      = rgt.getPreReleaseId();
                this->m_pre_release         = rgt.getPreRelease();
                this->m_build               = rgt.getBuild();
                this->m_is_valid            = rgt.isValid();
                this->m_is_stable           = rgt.isStable();
            }

            return *this;
        }

        friend bool operator== (version &lft, version &rgt)
        {
			return !(lft != rgt);
        }

        friend bool operator!= (version &lft, version &rgt)
        {
			return (lft > rgt) || (lft < rgt);
        }

        friend bool operator> (version &lft, version &rgt)
        {
            // Major
            if (lft.getMajor() < 0 && rgt.getMajor() >= 0)
            {
                return false;
            }

            if (lft.getMajor() >= 0 && rgt.getMajor() < 0)
            {
                return true;
            }

            if (lft.getMajor() > rgt.getMajor())
            {
                return true;
            }

            if (lft.getMajor() < rgt.getMajor())
            {
                return false;
            }


            // Minor
            if (lft.getMinor() < 0 && rgt.getMinor() >= 0)
            {
                return false;
            }

            if (lft.getMinor() >= 0 && rgt.getMinor() < 0)
            {
                return true;
            }

            if (lft.getMinor() > rgt.getMinor())
            {
                return true;
            }

            if (lft.getMinor() < rgt.getMinor())
            {
                return false;
            }


            // Patch
            if (lft.getPatch() < 0 && rgt.getPatch() >= 0)
            {
                return false;
            }

            if (lft.getPatch() >= 0 && rgt.getPatch() < 0)
            {
                return true;
            }

            if (lft.getPatch() > rgt.getPatch())
            {
                return true;
            }

            if (lft.getPatch() < rgt.getPatch())
            {
                return false;
            }
            
            // Pre release
			if (
				(lft.getPreReleaseType() == rgt.getPreReleaseType()) &&
				(lft.getPreReleaseId() == rgt.getPreReleaseId())
			)
			{
				return false;
			}

			if (
				(lft.getPreReleaseType() ==  rgt.getPreReleaseType()) &&
                (lft.getPreReleaseId().find_first_not_of("0123456789") == std::string::npos) &&
                (rgt.getPreReleaseId().find_first_not_of("0123456789") == std::string::npos)
            )
            {
                if (atoi(lft.getPreReleaseId().c_str()) > atoi(rgt.getPreReleaseId().c_str()))
                {
                    return true;
                }
				else
				{
					return false;
				}
            }

			if (
				(lft.getPreReleaseType() == rgt.getPreReleaseType()) &&
				(lft.getPreReleaseId().compare(rgt.getPreReleaseId()) > 0)
			)
			{
				return true;
			}

			if (lft.getPreReleaseType() > rgt.getPreReleaseType())
			{
				return true;
			}

            return false;
        }

        friend bool operator>= (version &lft, version &rgt)
        {
            return (lft > rgt) || (lft == rgt);
        }

        friend bool operator< (version &lft, version &rgt)
        {
            return (rgt > lft);
        }

        friend bool operator<= (version &lft, version &rgt)
        {
            return (lft < rgt) || (lft == rgt);
        }

        friend std::ostream& operator<< (std::ostream& out, const version& value)
        {
            out << value.getVersion();

            return out;
        }
    };

} // end semver namespace

#endif // VERSION_H
