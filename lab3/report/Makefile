report.pdf: report.tex
	@pdflatex *.tex
	@pdflatex *.tex
	@rm -f *.aux
	@rm -f *.log
	@rm -f *.toc

force: clean report.pdf

open: report.pdf
	@evince report.pdf &

clean: 
	@rm -f *.aux
	@rm -f *.log
	@rm -f *.pdf
	@rm -f *.toc
