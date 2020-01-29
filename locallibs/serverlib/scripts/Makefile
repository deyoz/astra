all: pkg

pkg: pkgIndex.tcl

pkgIndex.tcl: $(filter-out pkgIndex.tcl, $(wildcard *.tcl))
	echo "pkg_mkIndex . $^" | tclsh

clean:
	rm -f pkgIndex.tcl
