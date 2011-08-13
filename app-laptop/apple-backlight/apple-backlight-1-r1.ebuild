# Distributed under the terms of the BSD3 license

inherit toolchain-funcs

EAPI=3

DESCRIPTION="intel macbook backlight control userspace tool"
HOMEPAGE="https://github.com/zong-sharo/apple-backlight"

LICENSE=""
SLOT="0"
KEYWORDS="~amd64 ~x86"
IUSE="hibernate"

DEPEND=""
RDEPEND="hibernate? ( sys-power/hibernate-script )"

src_compile () {
	$(tc-getCXX) ${FILESDIR}/apple-backlight-${PV}.cpp -o apple-backlight || die
}

src_install () {
	exeinto /usr/bin
	doexe apple-backlight || die

	if use hibernate ; then
		dodir /etc/hibernate/scriptlets.d/
		insinto /etc/hibernate/scriptlets.d/
		newins "${FILESDIR}/apple_backlight_scriptlet" apple_backlight || die
	fi
}

pkg_postinst() {
	chmod u+s  /usr/bin/apple-backlight
}
