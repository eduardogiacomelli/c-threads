import type { Metadata } from "next";
import { IBM_Plex_Mono, IBM_Plex_Sans } from "next/font/google";
import "./globals.css";

const plexSans = IBM_Plex_Sans({
  variable: "--font-plex-sans",
  subsets: ["latin"],
  weight: ["400", "500", "600", "700"],
});

const plexMono = IBM_Plex_Mono({
  variable: "--font-plex-mono",
  subsets: ["latin"],
  weight: ["400", "500", "600"],
});

export const metadata: Metadata = {
  title: "C Machine",
  description:
    "Step through C programs and watch the stack, the heap and the threads move.",
};

/**
 * Theme is a class on <html>, set before first paint by this blocking
 * snippet. No provider and no React state: the toggle flips the class and
 * the CSS follows, which also means nothing here can desync during
 * hydration. (next-themes renders its script inside <body>, which React 19
 * rejects outright.)
 */
const THEME_BOOT = `try{var t=localStorage.getItem('cm-theme');
if(t==='light'){document.documentElement.classList.remove('dark')}
else if(t==='dark'||window.matchMedia('(prefers-color-scheme: dark)').matches){document.documentElement.classList.add('dark')}
}catch(e){}`;

export default function RootLayout({
  children,
}: Readonly<{ children: React.ReactNode }>) {
  return (
    <html lang="en" suppressHydrationWarning>
      <head>
        <script dangerouslySetInnerHTML={{ __html: THEME_BOOT }} />
      </head>
      <body className={`${plexSans.variable} ${plexMono.variable} antialiased`}>
        {children}
      </body>
    </html>
  );
}
