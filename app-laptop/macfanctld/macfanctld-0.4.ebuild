# Distributed under the terms of the BSD3 license

inherit bzr linux-info
EAPI=3

DESCRIPTION="Fan control daemon for MacBook"
HOMEPAGE="https://launchpad.net/macfanctld"
EBZR_REPO_URI="lp:macfanctld"
EBZR_REVISION="revid:mikael@sesamiq.com-20101106105803-7ni76wp89kr6mwef"

CONFIG_CHECK="~SENSORS_APPLESMC"

LICENSE="GPL-3"
SLOT="0"
KEYWORDS="~amd64 ~x86"
IUSE=""

DEPEND=""
RDEPEND=""

src_prepare () {
	sed -i "s|dh_installdirs||" Makefile
	}

src_install () {
	dosbin macfanctld
	doman "macfanctld.1"
	newinitd "${FILESDIR}/macfanctld.init" "macfanctld"
	insinto "/etc"
	doins "macfanctl.conf"
	}
