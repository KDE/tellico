#!/usr/bin/env python
# -*- coding: iso-8859-1 -*-

# ***************************************************************************
#    copyright            : (C) 2006-2009 by Mathias Monnerville
#    email                : tellico@monnerville.com
# ***************************************************************************
#
# ***************************************************************************
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of version 2 of the GNU General Public License as  *
# *   published by the Free Software Foundation;                            *
# *                                                                         *
# ***************************************************************************
#
# Version 0.6: 2009-03-04 (Thanks to R. Fischer and Henry-Nicolas Tourneur)
# * Fixed parsing issues (various RegExp issues due to allocine's HTML changes)
#
# Version 0.5: 2009-01-21 (Changes contributed by R. Fischer <fischer.tellico@free.fr>)
# * Added complete distribution of actors and roles, Genres, Nationalities, producers, composer and scenarist
# * Fixed the plot field that returned a wrong answer when no plot is available
# * Fixed a bug related to parameters encoding
#
# Version 0.4:
# * Fixed parsing errors: some fields in allocine's HTML pages have changed recently. Multiple actors and genres 
# could not be retrieved. Fixed bad http request error due to some changes in HTML code.
#
# Version 0.3:
# * Fixed parsing: some fields in allocine's HTML pages have changed. Movie's image could not be fetched anymore. Fixed.
# 
# Version 0.2:
# * Fixed parsing: allocine's HTML pages have changed. Movie's image could not be fetched anymore.
# 
# Version 0.1:
# * Initial release.

import sys, os, re, md5, random
import urllib, urllib2, time, base64
import xml.dom.minidom
import locale

XML_HEADER = """<?xml version="1.0" encoding="UTF-8"?>"""
DOCTYPE = """<!DOCTYPE tellico PUBLIC "-//Robby Stephenson/DTD Tellico V9.0//EN" "http://periapsis.org/tellico/dtd/v9/tellico.dtd">"""

VERSION = "0.6"

def genMD5():
	obj = md5.new()
	float = random.random()
	obj.update(str(float))
	return obj.hexdigest()

class BasicTellicoDOM:
	def __init__(self):
		self.__doc = xml.dom.minidom.Document()
		self.__root = self.__doc.createElement('tellico')
		self.__root.setAttribute('xmlns', 'http://periapsis.org/tellico/')
		self.__root.setAttribute('syntaxVersion', '9')
		
		self.__collection = self.__doc.createElement('collection')
		self.__collection.setAttribute('title', 'My Movies')
		self.__collection.setAttribute('type', '3')
		
		self.__fields = self.__doc.createElement('fields')
		# Add all default (standard) fields
		self.__dfltField = self.__doc.createElement('field')
		self.__dfltField.setAttribute('name', '_default')
		
		# Add a custom 'Collection' field
		self.__customField = self.__doc.createElement('field')
		self.__customField.setAttribute('name', 'titre-original')
		self.__customField.setAttribute('title', 'Original Title')
		self.__customField.setAttribute('flags', '0')
		self.__customField.setAttribute('category', unicode('Général', 'latin-1').encode('utf-8'))
		self.__customField.setAttribute('format', '1')
		self.__customField.setAttribute('type', '1')
		self.__customField.setAttribute('i18n', 'yes')
		
		self.__fields.appendChild(self.__dfltField)
		self.__fields.appendChild(self.__customField)
		self.__collection.appendChild(self.__fields)

		self.__images = self.__doc.createElement('images')

		self.__root.appendChild(self.__collection)
		self.__doc.appendChild(self.__root)

		# Current movie id
		self.__currentId = 0


	def addEntry(self, movieData):
		"""
		Add a movie entry
		"""
		d = movieData
		entryNode = self.__doc.createElement('entry')
		entryNode.setAttribute('id', str(self.__currentId))

		titleNode = self.__doc.createElement('title')
		titleNode.appendChild(self.__doc.createTextNode(unicode(d['title'], 'latin-1').encode('utf-8')))

		otitleNode = self.__doc.createElement('titre-original')
		otitleNode.appendChild(self.__doc.createTextNode(unicode(d['otitle'], 'latin-1').encode('utf-8')))

		yearNode = self.__doc.createElement('year')
		yearNode.appendChild(self.__doc.createTextNode(unicode(d['year'], 'latin-1').encode('utf-8')))

		genresNode = self.__doc.createElement('genres')
		for g in d['genres']:
			genreNode = self.__doc.createElement('genre')
			genreNode.appendChild(self.__doc.createTextNode(unicode(g, 'latin-1').encode('utf-8')))
			genresNode.appendChild(genreNode)

		natsNode = self.__doc.createElement('nationalitys')
		for g in d['nat']:
			natNode = self.__doc.createElement('nat')
			natNode.appendChild(self.__doc.createTextNode(unicode(g, 'latin-1').encode('utf-8')))
			natsNode.appendChild(natNode)

		castsNode = self.__doc.createElement('casts')
		i = 0
		while i < len(d['actors']):
			h = d['actors'][i]
			g = d['actors'][i+1]			
			castNode = self.__doc.createElement('cast')
			col1Node = self.__doc.createElement('column')
			col2Node = self.__doc.createElement('column')
			col1Node.appendChild(self.__doc.createTextNode(unicode(g, 'latin-1').encode('utf-8')))
			col2Node.appendChild(self.__doc.createTextNode(unicode(h, 'latin-1').encode('utf-8')))
			castNode.appendChild(col1Node)
			castNode.appendChild(col2Node)
			castsNode.appendChild(castNode)
			i = i + 2

		dirsNode = self.__doc.createElement('directors')
		for g in d['dirs']:
			dirNode = self.__doc.createElement('director')
			dirNode.appendChild(self.__doc.createTextNode(unicode(g, 'latin-1').encode('utf-8')))
			dirsNode.appendChild(dirNode)

		prodsNode = self.__doc.createElement('producers')
		for g in d['prods']:
			prodNode = self.__doc.createElement('producer')
			prodNode.appendChild(self.__doc.createTextNode(unicode(g, 'latin-1').encode('utf-8')))
			prodsNode.appendChild(prodNode)

		scensNode = self.__doc.createElement('writers')
		for g in d['scens']:
			scenNode = self.__doc.createElement('writer')
			scenNode.appendChild(self.__doc.createTextNode(unicode(g, 'latin-1').encode('utf-8')))
			scensNode.appendChild(scenNode)

		compsNode = self.__doc.createElement('composers')
		for g in d['comps']:
			compNode = self.__doc.createElement('composer')
			compNode.appendChild(self.__doc.createTextNode(unicode(g, 'latin-1').encode('utf-8')))
			compsNode.appendChild(compNode)

		timeNode = self.__doc.createElement('running-time')
		timeNode.appendChild(self.__doc.createTextNode(unicode(d['time'], 'latin-1').encode('utf-8')))

		allocineNode = self.__doc.createElement(unicode('allociné-link', 'latin-1').encode('utf-8'))
		allocineNode.appendChild(self.__doc.createTextNode(unicode(d['allocine'], 'latin-1').encode('utf-8')))

		plotNode = self.__doc.createElement('plot')
		plotNode.appendChild(self.__doc.createTextNode(unicode(d['plot'], 'latin-1').encode('utf-8')))

		if d['image']:
			imageNode = self.__doc.createElement('image')
			imageNode.setAttribute('format', 'JPEG')
			imageNode.setAttribute('id', d['image'][0])
			imageNode.setAttribute('width', '120')
			imageNode.setAttribute('height', '160')
			imageNode.appendChild(self.__doc.createTextNode(unicode(d['image'][1], 'latin-1').encode('utf-8')))

			coverNode = self.__doc.createElement('cover')
			coverNode.appendChild(self.__doc.createTextNode(d['image'][0]))

		for name in (	'titleNode', 'otitleNode', 'yearNode', 'genresNode', 'natsNode', 
						'castsNode', 'dirsNode', 'timeNode', 'allocineNode', 'plotNode', 
						'prodsNode', 'compsNode', 'scensNode' ):
			entryNode.appendChild(eval(name))

		if d['image']:
			entryNode.appendChild(coverNode)
			self.__images.appendChild(imageNode)

		self.__collection.appendChild(entryNode)
		
		self.__currentId += 1

	def printXML(self):
		"""
		Outputs XML content to stdout
		"""
		self.__collection.appendChild(self.__images)
		print XML_HEADER; print DOCTYPE
		print self.__root.toxml()


class AlloCineParser:
	def __init__(self):
		self.__baseURL 	= 'http://www.allocine.fr'
		self.__basePath = '/film/fichefilm_gen_cfilm'
		self.__castPath = '/film/casting_gen_cfilm'
		self.__searchURL= 'http://www.allocine.fr/recherche/?motcle=%s&f=3&rub=1'
		self.__movieURL = self.__baseURL + self.__basePath
		self.__castURL = self.__baseURL + self.__castPath

		# Define some regexps
		self.__regExps = { 	'title' 	: '<td><h1 class=".*?">(?P<title>.+?)</h1></td>',
							'dirs'		: 'Réalisé par <a.*?>(?P<step1>.+?)</a>.*?</h3>',
							'nat'		: '<h3 class=".*?">Film *(?P<nat>.+?)[\.]',
							'genres' 	: '<h3 class=".*?">Genre *:*(?P<step1>.+?)</h3>',
							'time' 		: '<h3 class=".*?">Durée *: *(?P<hours>[0-9])?h[ \.]*(?P<mins>[0-9]*)',
							'year' 		: '<h3 class=".*?">Année de production *: *(?P<year>[0-9]{4})',
							# Original movie title
							'otitle' 	: 'Titre original *: *<i>(?P<otitle>.+?)</i>',
							'plot'		: """(?s)<td valign="top" style="padding:10 0 0 0"><div align="justify"><h4> *(?P<plot>.*?) *</h4>""",
							'image'		: """<td valign="top" width="120".*?<img src="(?P<image>.+?)" border"""
		}

		self.__castRegExps = {	'role' 			: '.*?<td width="50%" valign="center" style="padding:2 0 2 0"><h5>(?P<role>[^<]*?)</h5></td>',
								'roleactor'		: '.*?<td width="50%" valign="center" style="padding:2 0 2 0"><h5>(?P<role>[^<]*?)</h5></td>.*?<a href="/personne/.*?" class="link1">(?P<someone>[^(]+?)\(*[0-9]*\)*</a>',
								'someone'		: '.*?<a href="/personne/.*?" class="link1">(?P<someone>[^(]+?)\(*[0-9]*\)*</a>',
								'debut_actors'	: '<tr><td.*?><h2[^>]*>Acteurs</h2>',
								'debut_prod'	: '.*?<td width="50%" valign="top" style="padding:5 2 0 0"><h5>Producteur</h5></td>',
								'fin_cat' 		: '.*?<td width="50%" valign="top" style="padding:5 2 0 0"><h5>',
								'debut_scenar'	: '.*?<td [^>]*><h5>Scénariste</h5></td>',
								'debut_compos'	: '.*?<td width="50%" valign="top" style="padding:5 2 0 0"><h5>Compositeur</h5></td>',
								'fin'			: '</table>'
		}
							

		self.__domTree = BasicTellicoDOM()

	def run(self, title):
		"""
		Runs the allocine.fr parser: fetch movie related links, then fills and prints the DOM tree
		to stdout (in tellico format) so that tellico can use it.
		"""

		loc = re.search('\.([^\.]*)', locale.setlocale(locale.LC_ALL, ''))
		if loc:
			local = loc.group(1)
		else:
			local = 'UTF-8'
		title = unicode(title, local).encode('latin-1')

		self.__getMovie(title)
		# Print results to stdout
		self.__domTree.printXML()

	def __getHTMLContent(self, url):
		"""
		Fetch HTML data from url
		"""

		u = urllib2.urlopen(url)
		self.__data = u.read()
		u.close()

	def __fetchMovieLinks(self):
		"""
		Retrieve all links related to movie
		"""

		matchList = re.findall("""<h4><a *href="%s=(?P<page>.*?\.html?)" *class="link1">(?P<title>.*?)</a>""" % self.__basePath, self.__data)
		if not matchList: return None

		return matchList

	def __fetchMovieInfo(self, url, url2):
		"""
		Looks for movie information
		"""

		self.__getHTMLContent(url)

		matches = data = {}

		for name, regexp in self.__regExps.iteritems():
			if name == 'image':
				matches[name] = re.findall(self.__regExps[name], self.__data, re.S | re.I)
			else:
				matches[name] = re.search(regexp, self.__data)

			if matches[name]:
				if name == 'title':
					data[name] = matches[name].group('title').strip()
				elif name == 'dirs':
					dirsList = re.sub('</?a.*?>', '', matches[name].group('step1')).split(',')
					data[name] = []
					for d in dirsList:
						data[name].append(d.strip())

				elif name == 'nat':
					natList = matches[name].group('nat').strip().split(', ')
					data[name] = []				
					for d in natList:
						data[name].append(d.strip())

				elif name == 'genres':
					genresList = re.sub('</?.*?>', '', matches[name].group('step1')).split(',')
					data[name] = []
					for d in genresList:
						data[name].append(d.strip())

				elif name == 'time':
					h, m = matches[name].group('hours'), matches[name].group('mins')
					if len(m) == 0:
						m = 0
					totmin = int(h)*60+int(m)
					data[name] = str(totmin)

				elif name == 'year':
					data[name] = matches[name].group('year').strip()

				elif name == 'otitle':
					data[name] = matches[name].group('otitle').strip()

				elif name == 'plot':
					data[name] = matches[name].group('plot').strip()

				# Image path
				elif name == 'image':
					# Save image to a temporary folder
					md5 = genMD5()
					imObj = urllib2.urlopen(matches[name][0].strip())
					img = imObj.read()
					imObj.close()
					imgPath = "/tmp/%s.jpeg" % md5
					try:
						f = open(imgPath, 'w')
						f.write(img)
						f.close()
					except:
						# Could be great if we can pass exit code and some message
						# to tellico in case of failure...
						pass

					data[name] = (md5 + '.jpeg', base64.encodestring(img))
					# Delete temporary image
					try:
						os.remove(imgPath)
					except:
						# Could be great if we can pass exit code and some msg
						# to tellico in case of failure...
						pass
			else:
				matches[name] = ''

		# Now looks for casting information
		self.__getHTMLContent(url2)
		page = self.__data.split('\n')

		d = zone = 0
		data['actors'] = []
		data['prods'] = []
		data['scens'] = []
		data['comps'] = []

		for line in page:
			d += 1
			if re.match(self.__castRegExps['fin'], line):
				zone = 0
			elif re.match(self.__castRegExps['debut_actors'], line):
				zone = 1
			elif re.match(self.__castRegExps['debut_prod'], line):
				zone = 2
			elif re.match(self.__castRegExps['debut_scenar'], line):
				zone = 3
			elif re.match(self.__castRegExps['debut_compos'], line):
				zone = 4
			elif zone == 1: 
				if re.match(self.__castRegExps['roleactor'], line2 + line):
					someone = re.search(self.__castRegExps['roleactor'], line2 + line).group('someone')
					role = re.search(self.__castRegExps['roleactor'], line2 + line).group('role')
					data['actors'].append(role)
					data['actors'].append(someone)
			elif zone == 2: # production
				if re.match(self.__castRegExps['fin_cat'], line):
					zone == 0
				else:
					if re.match(self.__castRegExps['someone'], line):
						data['prods'].append(re.search(self.__castRegExps['someone'], line).group('someone'))
			elif zone == 3: # scenario
				if re.match(self.__castRegExps['fin_cat'], line):
					zone = 0 
				elif re.match(self.__castRegExps['someone'], line):
					data['scens'].append(re.search(self.__castRegExps['someone'], line).group('someone'))
			elif zone == 4: # composition
				if re.match(self.__castRegExps['fin_cat'], line):
					zone = 0 
				elif re.match(self.__castRegExps['someone'], line):
					data['comps'].append(re.search(self.__castRegExps['someone'], line).group('someone'))
			line2 = line

		return data

	def __getMovie(self, title):
		if not len(title): return

		self.__title = title
		self.__getHTMLContent(self.__searchURL % urllib.quote(self.__title))

		# Get all links
		links = self.__fetchMovieLinks()

		# Now retrieve infos
		if links:
			for entry in links:
				data = self.__fetchMovieInfo( url = "%s=%s" % (self.__movieURL, entry[0]), url2 = "%s=%s" % (self.__castURL, entry[0]) )
				# Add allocine link (custom field)
				data['allocine'] = "%s=%s" % (self.__movieURL, entry[0])
				self.__domTree.addEntry(data)
		else:
			return None


def showUsage():
	print "Usage: %s movietitle" % sys.argv[0]
	sys.exit(1)

def main():
	if len(sys.argv) < 2:
		showUsage()

	parser = AlloCineParser()
	parser.run(sys.argv[1])

if __name__ == '__main__':
	main()
