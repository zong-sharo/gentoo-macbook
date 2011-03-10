# Copyright 1999-2011 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: $

#inherit git

EAPI=3

DESCRIPTION="intel macbook backlight control userspace tool"
HOMEPAGE="https://github.com/zong-sharo/apple-backlight"
#EGIT_REPO_URI="git://github.com/zong-sharo/apple-backlight.git"

LICENSE=""
SLOT="0"
KEYWORDS="~amd64 ~x86"
IUSE=""

DEPEND=""
RDEPEND="${DEPEND}"

src_compile () {
	g++ ${FILESDIR}/apple-backlight-${PV}.cpp -o apple-backlight
}

src_install () {
	exeinto /usr/bin
	doexe apple-backlight

}

pkg_postinst() {
	chmod u+s  /usr/bin/apple-backlight
}
