#!/usr/bin/env python
# -*- coding: iso-8859-1 -*-
# kate: replace-tabs off;
# ***************************************************************************
#    copyright            : (C) 2006-2010 by Mathias Monnerville
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
# Version 0.7.3: 2010-12-07 (Reported by Romain Henriet)
# * Fixed some regexp issues
# * Better handling of image parsing/fetching errors
#
# Version 0.7.2.1: 2010-07-27 (Reported by Romain Henriet)
# * Updated title match to allow searching without diacritical marks
#
# Version 0.7.2: 2010-05-27 (Reported by Romain Henriet)
# * Fixed bug preventing searches with accent marks
# * Added post-processing cleanup action to replace raw HTML entities with
#   their ISO Latin-1 replacement text
#
# Version 0.7.1: 2010-04-26 (Thanks to Romain Henriet <romain-devel@laposte.net>)
# * Fixed greedy regexp for genre.  Fixed nationality output. Add studio.
#
# Version 0.7: 2009-11-12
# * Allocine has a brand new website. All regexps were broken.
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

import sys, os, re, hashlib, random, types
import urllib, urllib2, time, base64
import xml.dom.minidom
import locale
try: import htmlentitydefs as htmlents
except ImportError:
	raise ImportError, 'Python 2.5+ required'

XML_HEADER = """<?xml version="1.0" encoding="UTF-8"?>"""
DOCTYPE = """<!DOCTYPE tellico PUBLIC "-//Robby Stephenson/DTD Tellico V9.0//EN" "http://periapsis.org/tellico/dtd/v9/tellico.dtd">"""

VERSION = "0.7.3"

def genMD5():
	float = random.random()
	return hashlib.md5(str(float)).hexdigest()

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
		titleNode.appendChild(self.__doc.createTextNode(d['title']))

		otitleNode = self.__doc.createElement('titre-original')
		otitleNode.appendChild(self.__doc.createTextNode(d['otitle']))

		yearNode = self.__doc.createElement('year')
		yearNode.appendChild(self.__doc.createTextNode(d['year']))

		genresNode = self.__doc.createElement('genres')
		for g in d['genres']:
			genreNode = self.__doc.createElement('genre')
			genreNode.appendChild(self.__doc.createTextNode(g))
			genresNode.appendChild(genreNode)

		studsNode = self.__doc.createElement('studios')
		for g in d['studio']:
			studNode = self.__doc.createElement('studio')
			studNode.appendChild(self.__doc.createTextNode(g))
			studsNode.appendChild(studNode)

		natsNode = self.__doc.createElement('nationalitys')
		for g in d['nat']:
			natNode = self.__doc.createElement('nationality')
			natNode.appendChild(self.__doc.createTextNode(g))
			natsNode.appendChild(natNode)

		castsNode = self.__doc.createElement('casts')
		i = 0
		while i < len(d['actors']):
			g = d['actors'][i]
			h = d['actors'][i+1]
			castNode = self.__doc.createElement('cast')
			col1Node = self.__doc.createElement('column')
			col2Node = self.__doc.createElement('column')
			col1Node.appendChild(self.__doc.createTextNode(g))
			col2Node.appendChild(self.__doc.createTextNode(h))
			castNode.appendChild(col1Node)
			castNode.appendChild(col2Node)
			castsNode.appendChild(castNode)
			i = i + 2

		dirsNode = self.__doc.createElement('directors')
		for g in d['dirs']:
			dirNode = self.__doc.createElement('director')
			dirNode.appendChild(self.__doc.createTextNode(g))
			dirsNode.appendChild(dirNode)

		prodsNode = self.__doc.createElement('producers')
		for g in d['prods']:
			prodNode = self.__doc.createElement('producer')
			prodNode.appendChild(self.__doc.createTextNode(g))
			prodsNode.appendChild(prodNode)

		scensNode = self.__doc.createElement('writers')
		for g in d['scens']:
			scenNode = self.__doc.createElement('writer')
			scenNode.appendChild(self.__doc.createTextNode(g))
			scensNode.appendChild(scenNode)

		compsNode = self.__doc.createElement('composers')
		for g in d['comps']:
			compNode = self.__doc.createElement('composer')
			compNode.appendChild(self.__doc.createTextNode(g))
			compsNode.appendChild(compNode)

		timeNode = self.__doc.createElement('running-time')
		timeNode.appendChild(self.__doc.createTextNode(d['time']))

		allocineNode = self.__doc.createElement(unicode('allociné-link', 'latin-1').encode('utf-8'))
		allocineNode.appendChild(self.__doc.createTextNode(d['allocine']))

		plotNode = self.__doc.createElement('plot')
		plotNode.appendChild(self.__doc.createTextNode(d['plot']))

		if d['image']:
			imageNode = self.__doc.createElement('image')
			imageNode.setAttribute('format', 'JPEG')
			imageNode.setAttribute('id', d['image'][0])
			imageNode.setAttribute('width', '120')
			imageNode.setAttribute('height', '160')
			imageNode.appendChild(self.__doc.createTextNode(d['image'][1]))

			coverNode = self.__doc.createElement('cover')
			coverNode.appendChild(self.__doc.createTextNode(d['image'][0]))

		for name in (	'titleNode', 'otitleNode', 'yearNode', 'genresNode', 'studsNode', 'natsNode',
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
		self.__searchURL= 'http://www.allocine.fr/recherche/?q=%s'
		self.__movieURL = self.__baseURL + self.__basePath
		self.__castURL = self.__baseURL + self.__castPath

		# Define some regexps
		self.__regExps = {
			'title' 	: '<div id="title.*?<span.*?>(?P<title>.+?)</span>',
			'dirs'		: """alis.*?par.*?<a.*?><span.*?>(?P<step1>.+?)</span></a>""",
			'nat'		: 'Nationalit.*?</span>(?P<nat>.+?)</td',
			'genres' 	: '<span class="lighten">.*?Genre.*?</span>(?P<step1>.+?)</td',
			'studio' 	: 'Distributeur</div>(?P<step1>.+?)</td',
			'time' 		: 'Dur.*?e *?:*?.*?(?P<hours>[0-9])h *(?P<mins>[0-9]*).*?Ann',
			'year' 		: 'Ann.*?e de production.*?<span.*?>(?P<year>[0-9]{4})</span>',
			'otitle' 	: 'Titre original *?:*?.*?<td>(?P<otitle>.+?)</td>',
			'plot'		: '<p itemprop="description">(?P<plot>.*?)</p>',
			'image'		: '<div class="poster">.*?<img src=\'(?P<image>http://.+?)\'.?',
		}

		self.__castRegExps = {
#			'roleactor'		: '<li.*?itemprop="actors".*?>.*?<span itemprop="name">(.*?)</span>.*?<p>.*?R.*?le : (?P<role>.*?)</p>.*?</li>',
			'roleactor'		: '<li.*?\/personne\/.*?">(.*?)</span>.*?<p.*?R.*?le : (?P<role>.*?)</p>.*?</li',
			'prods'			  : '<td>[\r\n\t]*Producteur[\r\n\t]*</td>.*?<span.*?>(.*?)</span>',
			'scens'			  : '<td>[\r\n\t]*Sc.*?nariste[\r\n\t]*</td>.*?<span.*?>(.*?)</span>',
			'comps'			  : '<td>[\r\n\t]*Compositeur[\r\n\t]*</td>.*?<span.*?>(.*?)</span>',
		}

		self.__domTree = BasicTellicoDOM()

	def run(self, title):
		"""
		Runs the allocine.fr parser: fetch movie related links, then fills and prints the DOM tree
		to stdout (in tellico format) so that tellico can use it.
		"""
		# the script needs the search string to be encoded in utf-8
		try:
			# first try system encoding
			title = unicode(title, sys.stdin.encoding or sys.getdefaultencoding())
		except UnicodeDecodeError:
			# on failure, fallback to 'latin-1'
			title = unicode(title, 'latin-1')

		# now encode for urllib
		title = title.encode('utf-8')
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

	def __fetchMovieLinks(self, title):
		"""
		Retrieve all links related to movie
		@param title Movie title
		"""
		tmp = re.findall("""<td.*?class=['"]totalwidth['"]>.*?<a *href=['"]%s=(?P<page>.*?\.html?)['"] *?>(?P<title>.*?)</a>""" % self.__basePath, self.__data, re.S | re.I)
		matchList = []
		for match in tmp:
			name = re.sub(r'([\r\n]+|<b>|</b>)', '', match[1])
			name = re.sub(r'<.*?>', '', name)
			name = re.sub(r'^ *', '', name)
			#if re.search(title, name, re.I):
			if len(name) > 0:
				matchList.append((match[0], name))

		if not matchList: return None
		return matchList

	def __fetchMovieInfo(self, url, url2):
		"""
		Looks for movie information
		"""
		self.__getHTMLContent(url)
		matches = data = {}

		for name, regexp in self.__regExps.iteritems():
			matches[name] = re.search(regexp, self.__data, re.S | re.I)

			if matches[name]:
				if name == 'title':
					data[name] = matches[name].group('title').strip()
				elif name == 'dirs':
					dirsList = re.sub('</?a.*?>', '', matches[name].group('step1')).split(',')
					data[name] = []
					for d in dirsList:
						data[name].append(d.strip())

				elif name == 'nat':
					natList = re.findall(r'<span class=".*?">(.*?)</span>', matches[name].group('nat'), re.DOTALL)
					data[name] = []
					for d in natList:
						data[name].append(d.strip().capitalize())

				elif name == 'genres':
					genresList = re.findall(r'<span itemprop="genre">(.*?)</span>', matches[name].group('step1'), re.DOTALL)
					data[name] = []
					for d in genresList:
						data[name].append(d.strip().capitalize())

				elif name == 'studio':
					studiosList = re.findall(r'<span itemprop="productionCompany">(.*?)</span>', matches[name].group('step1'))
					data[name] = []
					for d in studiosList:
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
					otitle = re.sub(r'([\r\n]+|<em>|</em>)', '', matches[name].group('otitle'))
					data[name] = otitle.strip()

				elif name == 'plot':
					data[name] = matches[name].group('plot').strip()
				# Cleans up any HTML entities
				data[name] = self.__cleanUp(data[name])

			else:
				matches[name] = ''

		# Image check
		try:
			imgtmp = re.findall(self.__regExps['image'], self.__data, re.S | re.I)
			matches['image'] = imgtmp[0]

			# Save image to a temporary folder
			md5 = genMD5()
			imObj = urllib2.urlopen(matches['image'].strip())
			img = imObj.read()
			imObj.close()
			imgPath = "/tmp/%s.jpeg" % md5
			f = open(imgPath, 'w')
			f.write(img)
			f.close()

			# Base64 encoding
			data['image'] = (md5 + '.jpeg', base64.encodestring(img))

			# Delete temporary image
			os.remove(imgPath)
		except:
			data['image'] = None

		# Now looks for casting information
		self.__getHTMLContent(url2)
		page = self.__data.split('\n')

		d = zone = 0
		data['actors'] = []
		data['prods'] = []
		data['scens'] = []
		data['comps'] = []

		# Actors
		subset = re.search(r'Acteurs et actrices.*$', self.__data, re.S | re.I)
		if not subset: return data
		subset = subset.group(0)
                #print subset
		roleactor = re.findall(self.__castRegExps['roleactor'], subset, re.S | re.I)
		for ra in roleactor:
                        #print ra
			data['actors'].append(re.sub(r'([\r\n\t]+)', '', ra[0]))
			data['actors'].append(re.sub(r'([\r\n\t]+)', '', ra[1]))

		# Producers, Scenarists, Composers
		for kind in ('prods', 'scens', 'comps'):
			data[kind] = [re.sub(r'([\r\n\t]+)', '', k).strip() for k in re.findall(self.__castRegExps[kind], subset, re.S | re.I)]

		return data

	def __cleanUp(self, data):
		"""
		Cleans up the string(s), replacing raw HTML entities with their
		ISO Latin-1 replacement text.
		@param data string or list of strings
		"""
		if type(data) == types.ListType:
			for s in data:
				for k, v in htmlents.entitydefs.iteritems():
					s = s.replace("&%s;" % k, v)
		elif type(data) == types.StringType or type(data) == types.UnicodeType:
			for k, v in htmlents.entitydefs.iteritems():
				data = data.replace("&%s;" % k, v)
		return data

	def __getMovie(self, title):
		if not len(title): return

		self.__title = title
		self.__getHTMLContent(self.__searchURL % urllib.quote(self.__title))

		# Get all links
		links = self.__fetchMovieLinks(title)

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
